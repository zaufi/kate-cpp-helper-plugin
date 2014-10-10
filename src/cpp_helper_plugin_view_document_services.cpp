/**
 * \file
 *
 * \brief Class \c kate::CppHelperPluginView (implementation part II)
 *
 * Kate C++ Helper Plugin view methods related to documents and processing
 *
 * \date Sat Nov 23 15:14:43 MSK 2013 -- Initial design
 */
/*
 * Copyright (C) 2011-2013 Alex Turbov, all rights reserved.
 * This is free software. It is licensed for use, modification and
 * redistribution under the terms of the GNU General Public License,
 * version 3 or later <http://gnu.org/licenses/gpl.html>
 *
 * KateCppHelperPlugin is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KateCppHelperPlugin is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Project specific includes
#include <src/cpp_helper_plugin_view.h>
#include <src/cpp_helper_plugin.h>
#include <src/choose_from_list_dialog.h>
#include <src/utils.h>

// Standard includes
#include <kate/application.h>
#include <kate/documentmanager.h>
#include <kate/mainwindow.h>
#include <KDE/KPassivePopup>
#include <QtCore/QDirIterator>
#include <QtCore/QFileInfo>
#include <QtGui/QClipboard>
#include <QtGui/QStandardItemModel>
#include <QtGui/QToolTip>

namespace kate { namespace {
const QStringList HDR_EXTENSIONS = QStringList() << "h" << "hh" << "hpp" << "hxx" << "H";
const QStringList SRC_EXTENSIONS = QStringList() << "c" << "cc" << "cpp" << "cxx" << "C" << "inl";
}                                                           // anonymous namespace

//BEGIN SLOTS
/**
 * Do same things as <em>Open Header</em> plugin do... but little better ;-)
 *
 * In general, header and implementation differ only in a file extension. There is a few extensions
 * possible for each: \c .h, \c .hh, \c .hpp, \c .hxx, \c .H and \c .c, \c .cc, \c .cpp, \c .cxx.
 *
 * Considering location, there is few cases possible:
 * \li both are placed in the same directory
 * \li header located somewhere else than implementation file. For example all translation units
 * are placed into \c ${project}/src and headers into \c ${project}/include. Depending on \c #include
 * style there are few cases possible also: whole project have only a source root as \c #include path,
 * or every particular directory must include all depended dirs as \c -I compiler keys, so in translation
 * units \c #include can use just a header name w/o any path preceding (this way lead to include-paths-hell).
 * \li implementation may have slightly differ name than header, but both have smth in common.
 * For example if some header \c blah.hh define some really big class, sometime implementation
 * spreaded across few files named after the header. Like \c blah_1.cc, \c blah_2.cc & etc.
 *
 * So it would be not so easy to handle all those cases. But here is one more case exists:
 * one of good practices told us to \c #include implementation as first header in all translation
 * units, so it would be possible to find a particular header easy if we may assume this fact...
 *
 * And final algorithm will to the following:
 * \li if we r in an implementation file and checkbox <em>open first #include file</em> checked,
 * then just open it, or ...
 * \li try to open counterpart file substituting different extensions at the same location
 * as original document placed, or (in case of failure)
 * \li if current file is a source, try to find a corresponding header file (with different extensions)
 * in all configured session dirs, or
 * \li if current file is a header, try to find a corresponding source in a directories with
 * other sources already opened. Here is a big chance that in one of that dirs we will find a
 * required implementation...
 * \li anything else? TBD
 *
 * \todo Make extension lists configurable. Better to read them from kate's configuration somehow...
 *
 * \todo Validate for duplicates removal
 */
void CppHelperPluginView::switchIfaceImpl()
{
    assert(
        "Active view suppose to be valid at this point! Am I wrong?"
      && mainWindow()->activeView()
      );
    // Ok, trying case 1
    auto* const doc = mainWindow()->activeView()->document();
    auto url = doc->url();
    if (!url.isValid() || url.isEmpty())
        return;

    auto info = QFileInfo{url.toLocalFile()};
    auto extension = info.suffix();

    kDebug(DEBUG_AREA) << "Current file ext: " <<  extension;

    const auto& active_doc_path = info.absolutePath();
    const auto& active_doc_name = info.completeBaseName();
    QStringList candidates;
    bool is_implementation_file;
    if ( (is_implementation_file = SRC_EXTENSIONS.contains(extension)) )
        candidates = findCandidatesAt(active_doc_name, active_doc_path, HDR_EXTENSIONS);
    else if (HDR_EXTENSIONS.contains(extension))
        candidates = findCandidatesAt(active_doc_name, active_doc_path, SRC_EXTENSIONS);
    else                                                    // Dunno what is this...
        /// \todo Show some SPAM?
        return;                                             // ... just leave!

    const auto& extensions_to_try =
        is_implementation_file ? HDR_EXTENSIONS : SRC_EXTENSIONS;

    kDebug(DEBUG_AREA) << "open src/hrd: stage1: found candidates: " << candidates;

    if (is_implementation_file)
    {
        // Lets try to find smth in configured sessions dirs
        // NOTE This way useful only if the current file is a source one!
        for (const auto& dir : m_plugin->config().sessionDirs())
            for (const auto& c : findCandidatesAt(active_doc_name, dir, extensions_to_try))
                if (!candidates.contains(c))
                    candidates.push_back(c);

        // Should we consider first #include in a document?
        kDebug(DEBUG_AREA) << "open src/hdr: shouldOpenFirstInclude=" << m_plugin->config().shouldOpenFirstInclude();
        if (m_plugin->config().shouldOpenFirstInclude())
        {
            kDebug(DEBUG_AREA) << "open src/hdr: open first #include enabled";
            // Try to find first #include in this (active) document
            QString first_header_name;
            for (auto i = 0; i < doc->lines() && first_header_name.isEmpty(); i++)
            {
                const auto& line_str = doc->line(i);
                kate::IncludeParseResult r = parseIncludeDirective(line_str, false);
                if (r.range.isValid())
                {
                    r.range.setBothLines(i);
                    first_header_name = doc->text(r.range);
                }
            }
            kDebug(DEBUG_AREA) << "open src/hrd: first include file:" << first_header_name;
            // Is active document has some #includes?
            if (!first_header_name.isEmpty())
            {
                // Ok, try to find it among session dirs
                QStringList files;
                findFiles(first_header_name, m_plugin->config().sessionDirs(), files);
                kDebug(DEBUG_AREA) << "* candidates: " << candidates;
                for (const auto& c : files)
                {
                    kDebug(DEBUG_AREA) << "** consider: " << c;
                    if (!candidates.contains(c))
                        candidates.push_back(c);
                }
            }
        }
    }
    else
    {
        // Lets try to find smth in dirs w/ already opened source files
        // NOTE This way useful only if the current file is a header one!
        // 0) Collect paths of already opened surce files
        QStringList src_paths;
        for (auto* current_doc : m_plugin->application()->documentManager()->documents())
        {
            auto current_doc_uri = current_doc->url();
            if (current_doc_uri.isValid() && !current_doc_uri.isEmpty())
            {
                auto doc_file_info = QFileInfo{current_doc_uri.toLocalFile()};
                const auto& current_path = doc_file_info.absolutePath();
                // Append current path only if the document is a source and no such path
                // collected yet...
                if (SRC_EXTENSIONS.contains(doc_file_info.suffix()) && !src_paths.contains(current_path))
                    src_paths.push_back(current_path);
            }
        }
        kDebug(DEBUG_AREA) << "open src/hrd: stage2: sources paths: " << src_paths;
        // Ok,  try to find smth in collected dirs
        for (const auto& dir : src_paths)
            for (const auto& c : findCandidatesAt(active_doc_name, dir, extensions_to_try))
                if (!candidates.contains(c))
                    candidates.push_back(c);

        kDebug(DEBUG_AREA) << "open src/hrd: stage1: found candidates: " << candidates;

        if (m_plugin->config().useWildcardSearch())
        {
            // And finally: try to find alternative source file
            // names using basename*.cc, basename*.cpp & so on...

            // Ok, now try to find more sources in collected paths using wildcards
            src_paths.push_front(active_doc_path);      // Do not forget about the current document's dir
            // Form filters list
            QStringList filters;
            for (const auto& ext : extensions_to_try)
                filters << (active_doc_name + "*." + ext);
            kDebug(DEBUG_AREA) << "open src/hrd: stage3: filters ready: " <<  filters;
            for (const auto& dir : src_paths)
            {
                for (
                    QDirIterator dir_it{
                        dir
                      , filters
                      , QDir::Files | QDir::NoDotAndDotDot | QDir::Readable | QDir::CaseSensitive
                      }
                  ; dir_it.hasNext()
                  ;)
                {
                    const auto& file = dir_it.next();
                    kDebug(DEBUG_AREA) << "open src/hrd: stage3: found " << file;
                    if (!candidates.contains(file))
                        candidates.push_back(file);
                }
            }
        }
    }
    kDebug(DEBUG_AREA) << "open src/hrd: final candidates: " << candidates;

    /// \todo More ways to find candidates...

    if (candidates.isEmpty())
    {
        KPassivePopup::message(
            i18n("Error")
          , i18n(
              "<qt>Unable to find a corresponding header/source for `<tt>%1</tt>'.</qt>"
            , url.toLocalFile()
            )
          , qobject_cast<QWidget*>(this)
          );
    }
    else if (candidates.size() == 1)
    {
        // No ambiguity! The only candidate! -- We r lucky! :-)
        openFile(candidates.first());
    }
    else
    {
        // Let user to choose what to open...
        openFile(ChooseFromListDialog::selectHeaderToOpen(qobject_cast<QWidget*>(this), candidates));
    }
}

void CppHelperPluginView::copyInclude()
{
    assert(
        "Active view suppose to be valid at this point! Am I wrong?"
      && mainWindow()->activeView()
      );

    const auto* view = mainWindow()->activeView();
    const auto& uri = view->document()->url();
    auto current_dir = uri.directory();
    QString longest_matched;
    auto open = QChar{m_plugin->config().useLtGt() ? '<' : '"'};
    auto close = QChar{m_plugin->config().useLtGt() ? '>' : '"'};
    kDebug(DEBUG_AREA) << "Got document name: " << uri << ", type: " << view->document()->mimeType();
    // Try to match local (per session) dirs first
    for (const auto& dir : m_plugin->config().sessionDirs())
        if (current_dir.startsWith(dir) && longest_matched.length() < dir.length())
            longest_matched = dir;
    if (longest_matched.isEmpty())
    {
        /// \todo Make it configurable
        open = '<';
        close = '>';
        // Try to match global dirs next
        for (const QString& dir : m_plugin->config().systemDirs())
            if (current_dir.startsWith(dir) && longest_matched.length() < dir.length())
                longest_matched = dir;
    }
    const auto is_suitable_document =
        isSuitableDocument(view->document()->mimeType(), view->document()->highlightingMode());
    QString text;
    if (!longest_matched.isEmpty())
    {
        if (is_suitable_document)
        {
            kDebug(DEBUG_AREA) << "current_dir=" << current_dir << ", lm=" << longest_matched;
            int count = longest_matched.size();
            for (; count < current_dir.size() && current_dir[count] == '/'; ++count) {}
            current_dir.remove(0, count);
            kDebug(DEBUG_AREA) << "current_dir=" << current_dir << ", lm=" << longest_matched;
            if (!current_dir.isEmpty() && !current_dir.endsWith('/'))
                current_dir += '/';
            text = QString("#include %1%2%3")
              .arg(open)
              .arg(current_dir + uri.fileName())
              .arg(close);
        }
        else text = uri.prettyUrl();
    }
    else
    {
        if (is_suitable_document)
            text = QString("#include \"%1\"").arg(uri.toLocalFile());
        else text = uri.prettyUrl();
    }
    kDebug(DEBUG_AREA) << "Result:" << text;
    if (!text.isEmpty())
        QApplication::clipboard()->setText(text);
}

/**
 * Action to open a header under cursor
 */
void CppHelperPluginView::openHeader()
{
    assert(
        "Active view suppose to be valid at this point! Am I wrong?"
      && mainWindow()->activeView()
      );

    QString filename;                                       // Name of header under cursor
    QStringList candidates;                                 // List of candidates (if any)

    auto* doc = mainWindow()->activeView()->document();

    auto result = findIncludeFilenameNearCursor();
    kDebug(DEBUG_AREA) << "findIncludeFilenameNearCursor() = " << result.range;

    if (!result.range.isEmpty())                            // Is any valid #include found?
    {
        filename = doc->text(result.range).trimmed();       // Assign filename under cursor for further use
        if (!filename.isEmpty())
        {
            // Try to find an absolute path to given filename
            candidates = findFileLocations(filename, result.type == IncludeStyle::local);
            candidates.removeDuplicates();
            kDebug(DEBUG_AREA) << "Found candidates: " << candidates;
        }
    }

    // If there is no ambiguity, then just emit a signal to open the file
    if (candidates.size() == 1)
    {
        openFile(candidates.first());
        return;
    }
    else if (candidates.isEmpty())                          // It seems nothing has found!
    {
        // Scan current document for #include files to offer other files
        for (int i = 0; i < doc->lines(); i++)
        {
            const auto& line_str = doc->line(i);
            auto r = parseIncludeDirective(line_str, false);
            if (r.range.isValid())
            {
                r.range.setBothLines(i);
                // Try to resolve relative filename to absolute
                candidates << findFileLocations(doc->text(r.range), r.type == IncludeStyle::local);
            }
        }
        candidates.removeDuplicates();

        // Ok form an error message if we were failed even on this
        auto error_text = filename.isEmpty()
          ? QString()
          : i18nc(
              "@info:tooltip"
            , "Unable to find the file: <filename>%1</filename>."
            , filename
            )
          + (candidates.isEmpty()
              ? QString()
              : i18nc(
                  "@info:tooltip"
                , "<p>Here is a list of <icode>#included</icode> headers in the current file...</p>"
                )
              )
          ;
        if (!error_text.isEmpty())
            KPassivePopup::message(
                i18nc("@title:window", "Error")
              , "<qt>" + error_text + "</qt>"
              , qobject_cast<QWidget*>(this)
              );
    }

    // Check candidates again: now show a dialog even if
    // there is only one header found after scan, because
    // it doesn't match anyway to the `filename`...
    if (!candidates.isEmpty())
        openFile(ChooseFromListDialog::selectHeaderToOpen(qobject_cast<QWidget*>(this), candidates));
}

/**
 * \note This function assume that given file is really exists.
 */
void CppHelperPluginView::openFile(const KUrl& file, const KTextEditor::Cursor cursor, const bool track_location)
{
    if (file.isEmpty()) return;                             // Nothing to do if no file specified

    kDebug(DEBUG_AREA) << "Going to open " << file;

    auto* const new_doc = m_plugin->application()->documentManager()->openUrl(file);
    /// \todo How to reuse \c isPresentAndReadable() from \c utils.h here?
    auto fi = QFileInfo{file.toLocalFile()};
    if (fi.isReadable())
    {
        // Remember a previous location if requested
        if (track_location)
        {
            auto* const prev_view = mainWindow()->activeView();
            auto prev_doc_url = prev_view->document()->url();
            const auto prev_cursor = prev_view->cursorPosition();
            const auto state_changed = m_recent_locations.empty();
            m_recent_locations.emplace(std::move(prev_doc_url), prev_cursor.line(), prev_cursor.column());
            if (state_changed)
                stateChanged("empty_locations_stack", KXMLGUIClient::StateReverse);
        }

        // Set RO status and cursor position
        new_doc->setReadWrite(fi.isWritable());
        mainWindow()->activateView(new_doc);
        if (cursor.isValid())
            mainWindow()->activeView()->setCursorPosition(cursor);
    }
    else
    {
        m_plugin->application()->documentManager()->closeDocument(new_doc);
        KPassivePopup::message(
            i18nc("@title:window", "Open error")
          , i18nc("@info:tooltip", "File <filename>%1</filename> is not readable", file.toLocalFile())
          , qobject_cast<QWidget*>(this)
          );
    }
}

void CppHelperPluginView::needTextHint(const KTextEditor::Cursor& pos, QString& text)
{
    assert(
        "Active view suppose to be valid at this point! Am I wrong?"
      && mainWindow()->activeView()
      );

    kDebug(DEBUG_AREA) << "Text hint requested at " << pos;

    auto* view = mainWindow()->activeView();                // get current view
    auto* doc = view->document();                           // get current document
    // Is current file can have some hints?
    if (isSuitableDocument(doc->mimeType(), doc->highlightingMode()))
    {
        const auto& di = m_plugin->getDocumentInfo(doc);
        text = di.getPossibleProblemText(pos.line());
    }
}

/**
 * To transform `#include <>` into `#include ""`:
 * - get 
 */
void CppHelperPluginView::toggleIncludeStyle()
{
    assert(
        "Active view suppose to be valid at this point! Am I wrong?"
      && mainWindow()->activeView()
      );
    // Ok, trying case 1
    auto* const doc = mainWindow()->activeView()->document();
}
//END SLOTS

//BEGIN Utility (private) functions
/**
 * Funtction to gather a list of candidates when switching header/implementation.
 * \param name filename to lookup header/implementation for
 * \param path path to check for candidates
 * \param extensions list of extensions to try
 * \return list of possible candidates
 */
QStringList CppHelperPluginView::findCandidatesAt(
    const QString& name
  , const QString& path
  , const QStringList& extensions
  )
{
    QStringList result;
    for (const auto& ext : extensions)
    {
        /// \todo Is there smth like \c boost::filesystem::path?
        auto filename = QDir::cleanPath(path + "/" + name + "." + ext);
        kDebug(DEBUG_AREA) << "open src/hrd: trying " << filename;
        if (isPresentAndReadable(filename))
            result.push_back(filename);
    }

    return result;
}

QStringList CppHelperPluginView::findFileLocations(const QString& filename, const bool is_local)
{
    assert(
        "Active view suppose to be valid at this point! Am I wrong?"
      && mainWindow()->activeView()
      );

    // Check CWD as well, if local #include or configuration option
    auto aux_paths = QStringList{};
    if (is_local || m_plugin->config().useCwd())
        aux_paths << QFileInfo(
            mainWindow()->activeView()->document()->url().path()
          ).dir().absolutePath();

    // Try to find full filename to open
    auto candidates = findHeader(
        filename
      , aux_paths
      , m_plugin->config().sessionDirs()
      , m_plugin->config().systemDirs()
      );
    candidates.removeDuplicates();                          // Remove possible duplicates
    return candidates;
}

IncludeParseResult CppHelperPluginView::findIncludeFilenameNearCursor() const
{
    assert(
        "Active view suppose to be valid at this point! Am I wrong?"
      && mainWindow()->activeView()
      );

    KTextEditor::Range range;                               // default range is empty
    auto* view = mainWindow()->activeView();
    if (!view || !view->cursorPosition().isValid())
        // do nothing if no view or valid cursor
        return {range};

    // Return selected text if any
    if (view->selection())
        return {view->selectionRange()};

    // Obtain a line under cursor
    const auto line = view->cursorPosition().line();
    const auto line_str = view->document()->line(line);

    // Check if current line starts w/ #include
    kate::IncludeParseResult r = parseIncludeDirective(line_str, false);
    if (r.range.isValid())
    {
        r.range.setBothLines(line);
        kDebug(DEBUG_AREA) << "Ok, found #include directive:" << r.range;
        return r;
    }

    // No #include parsed... fallback to the default way:
    // just search a word under cursor...

    // Get cursor line/column
    const auto col = view->cursorPosition().column();

    // NOTE Make sure cursor withing a line
    // (dunno is it possible to have a cursor far than a line length...
    // in block selection mode for example)
    auto start = qMax(qMin(col, line_str.length() - 1), 0);
    auto end = start;
    kDebug(DEBUG_AREA) << "Arghh... trying w/ word under cursor starting from" << start;

    // Seeking for start of current word
    for (; start > 0; --start)
    {
        if (line_str[start].isSpace() || line_str[start] == '<' || line_str[start] == '"')
        {
            start++;
            break;
        }
    }
    // Seeking for end of the current word
    while (end < line_str.length() && !line_str[end].isSpace() && line_str[end] != '>' && line_str[end] != '"')
        ++end;

    return {{line, start, line, end}};
}
//END Utility (private) functions
}                                                           // namespace kate
// kate: hl C++/Qt;
