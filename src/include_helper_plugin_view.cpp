/**
 * \file
 *
 * \brief Class \c kate::IncludeHelperPluginView (implementation)
 *
 * \date Mon Feb  6 06:17:32 MSK 2012 -- Initial design
 */
/*
 * KateIncludeHelperPlugin is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KateIncludeHelperPlugin is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Project specific includes
#include <src/include_helper_plugin_view.h>
#include <src/clang_code_completion_model.h>
#include <src/include_helper_plugin_completion_model.h>
#include <src/include_helper_plugin.h>
#include <src/utils.h>

// Standard includes
#include <kate/application.h>
#include <kate/documentmanager.h>
#include <kate/mainwindow.h>
#include <KTextEditor/CodeCompletionInterface>
#include <KTextEditor/Document>
#include <KActionCollection>
#include <KLocalizedString>                                 /// \todo Where is \c i18n() defiend?
#include <KPassivePopup>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtCore/QFileInfo>
#include <QtCore/QDirIterator>
#include <cassert>

namespace kate { namespace {
const QStringList HDR_EXTENSIONS = QStringList() << "h" << "hh" << "hpp" << "hxx" << "H";
const QStringList SRC_EXTENSIONS = QStringList() << "c" << "cc" << "cpp" << "cxx" << "C" << "inl";
}                                                           // anonymous namespace


//BEGIN IncludeHelperPluginView
IncludeHelperPluginView::IncludeHelperPluginView(
    Kate::MainWindow* mw
  , const KComponentData& data
  , IncludeHelperPlugin* plugin
  )
  : Kate::PluginView(mw)
  , Kate::XMLGUIClient(data)
  , m_plugin(plugin)
  , m_open_header(actionCollection()->addAction("file_open_included_header"))
  , m_copy_include(actionCollection()->addAction("edit_copy_include"))
  , m_switch(actionCollection()->addAction("file_open_switch_iface_impl"))
{
    m_open_header->setText(i18n("Open Header Under Cursor"));
    m_open_header->setShortcut(QKeySequence(Qt::Key_F10));
    connect(m_open_header, SIGNAL(triggered(bool)), this, SLOT(openHeader()));

    m_switch->setText(i18n("Open Header/Implementation"));
    m_switch->setShortcut(QKeySequence(Qt::Key_F12));
    connect(m_switch, SIGNAL(triggered(bool)), this, SLOT(switchIfaceImpl()));

    m_copy_include->setText(i18n("Copy #include to Clipboard"));
    m_copy_include->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_F10));
    connect(m_copy_include, SIGNAL(triggered(bool)), this, SLOT(copyInclude()));

    // On viewCreated we have to registerCompletionModel...
    connect(
        mainWindow()
      , SIGNAL(viewCreated(KTextEditor::View*))
      , this
      , SLOT(viewCreated(KTextEditor::View*))
      );

    // We want to enable/disable open header action depending on
    // mime-type of the current document, so we have to subscribe
    // to view changes...
    connect(mainWindow(), SIGNAL(viewChanged()), this, SLOT(viewChanged()));
    mainWindow()->guiFactory()->addClient(this);
}

IncludeHelperPluginView::~IncludeHelperPluginView() {
    mainWindow()->guiFactory()->removeClient(this);
}

void IncludeHelperPluginView::readSessionConfig(KConfigBase*, const QString& groupPrefix)
{
    kDebug() << "** VIEW **: Reading session config: " << groupPrefix;
}

void IncludeHelperPluginView::writeSessionConfig(KConfigBase*, const QString& groupPrefix)
{
    kDebug() << "** VIEW **: Writing session config: " << groupPrefix;
}

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
void IncludeHelperPluginView::switchIfaceImpl()
{
    // Ok, trying case 1
    KTextEditor::Document* doc = mainWindow()->activeView()->document();
    KUrl url = doc->url();
    if (!url.isValid() || url.isEmpty())
        return;

    QFileInfo info(url.toLocalFile());
    QString extension = info.suffix();

    kDebug() << "Current file ext: " <<  extension;

    const QString& active_doc_path = info.absolutePath();
    const QString& active_doc_name = info.completeBaseName();
    QStringList candidates;
    bool is_implementation_file;
    if ( (is_implementation_file = SRC_EXTENSIONS.contains(extension)) )
        candidates = findCandidatesAt(active_doc_name, active_doc_path, HDR_EXTENSIONS);
    else if (HDR_EXTENSIONS.contains(extension))
        candidates = findCandidatesAt(active_doc_name, active_doc_path, SRC_EXTENSIONS);
    else                                                    // Dunno what is this...
        /// \todo Show some SPAM?
        return;                                             // ... just leave!

    const QStringList& extensions_to_try =
        is_implementation_file ? HDR_EXTENSIONS : SRC_EXTENSIONS;

    kDebug() << "open src/hrd: stage1: found candidates: " << candidates;

    if (is_implementation_file)
    {
        // Lets try to find smth in configured sessions dirs
        // NOTE This way useful only if the current file is a source one!
        for (const QString& dir : m_plugin->config().sessionDirs())
            for (const QString& c : findCandidatesAt(active_doc_name, dir, extensions_to_try))
                if (!candidates.contains(c))
                    candidates.push_back(c);

        // Should we consider first #include in a document?
        kDebug() << "open src/hdr: shouldOpenFirstInclude=" << m_plugin->config().shouldOpenFirstInclude();
        if (m_plugin->config().shouldOpenFirstInclude())
        {
            kDebug() << "open src/hdr: open first #include enabled";
            // Try to find first #include in this (active) document
            QString first_header_name;
            for (int i = 0; i < doc->lines() && first_header_name.isEmpty(); i++)
            {
                const QString& line_str = doc->line(i);
                kate::IncludeParseResult r = parseIncludeDirective(line_str, false);
                if (r.m_range.isValid())
                {
                    r.m_range.setBothLines(i);
                    first_header_name = doc->text(r.m_range);
                }
            }
            kDebug() << "open src/hrd: first include file:" << first_header_name;
            // Is active document has some #includes?
            if (!first_header_name.isEmpty())
            {
                // Ok, try to find it among session dirs
                QStringList files;
                findFiles(first_header_name, m_plugin->config().sessionDirs(), files);
                kDebug() << "* candidates: " << candidates;
                for (const QString& c : files)
                {
                    kDebug() << "** consider: " << c;
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
        for (KTextEditor::Document* current_doc : m_plugin->application()->documentManager()->documents())
        {
            KUrl current_doc_uri = current_doc->url();
            if (current_doc_uri.isValid() && !current_doc_uri.isEmpty())
            {
                QFileInfo doc_file_info(current_doc_uri.toLocalFile());
                const QString& current_path = doc_file_info.absolutePath();
                // Append current path only if the document is a source and no such path
                // collected yet...
                if (SRC_EXTENSIONS.contains(doc_file_info.suffix()) && !src_paths.contains(current_path))
                    src_paths.push_back(current_path);
            }
        }
        kDebug() << "open src/hrd: stage2: sources paths: " << src_paths;
        // Ok,  try to find smth in collected dirs
        for (const QString& dir : src_paths)
            for (const QString& c : findCandidatesAt(active_doc_name, dir, extensions_to_try))
                if (!candidates.contains(c))
                    candidates.push_back(c);

        kDebug() << "open src/hrd: stage1: found candidates: " << candidates;

        if (m_plugin->config().useWildcardSearch())
        {
            // And finally: try to find alternative source file
            // names using basename*.cc, basename*.cpp & so on...

            // Ok, now try to find more sources in collected paths using wildcards
            src_paths.push_front(active_doc_path);      // Do not forget about the current document's dir
            // Form filters list
            QStringList filters;
            for (const QString& ext : extensions_to_try)
                filters << (active_doc_name + "*." + ext);
            kDebug() << "open src/hrd: stage3: filters ready: " <<  filters;
            for (const QString& dir : src_paths)
            {
                for (
                    QDirIterator dir_it(
                        dir
                    , filters
                    , QDir::Files|QDir::NoDotAndDotDot|QDir::Readable|QDir::CaseSensitive
                    )
                ; dir_it.hasNext()
                ;
                )
                {
                    dir_it.next();
                    const QString& file = dir_it.fileInfo().absoluteFilePath();
                    kDebug() << "open src/hrd: stage3: found " << file;
                    if (!candidates.contains(file))
                        candidates.push_back(file);
                }
            }
        }
    }
    kDebug() << "open src/hrd: final candidates: " << candidates;

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

QStringList IncludeHelperPluginView::findCandidatesAt(
    const QString& name
  , const QString& path
  , const QStringList& extensions
  )
{
    QStringList result;
    for (const QString& ext : extensions)
    {
        /// \todo Is there smth like \c boost::filesystem::path?
        QString filename = QDir::cleanPath(path + "/" + name + "." + ext);
        kDebug() << "open src/hrd: trying " << filename;
        if (isPresentAndReadable(filename))
            result.push_back(filename);
    }

    return result;
}

void IncludeHelperPluginView::openHeader()
{
    QStringList candidates;
    QString filename;
    KTextEditor::Document* doc = mainWindow()->activeView()->document();

    KTextEditor::Range r = currentWord();
    kDebug() << "currentWord() = " << r;
    if (!r.isEmpty())
    {
        // NOTE Non empty range means that main window is valid
        // and active view present as well!
        assert("Main window and active view expected to be valid!" && mainWindow()->activeView());

        filename = doc->text(r).trimmed();
        assert("Getting text on non-empty range should return smth!" && !filename.isEmpty());

        // Try to find an absolute path to given filename
        candidates = findFileLocations(filename);
        kDebug() << "Found candidates: " << candidates;
    }

    // If there is no ambiguity, then just emit a signal to open the file
    if (candidates.size() == 1)
    {
        openFile(candidates.first());
        return;
    }
    else if (candidates.isEmpty())
    {
        // Scan current document for #include files
        for (int i = 0; i < doc->lines(); i++)
        {
            const QString& line_str = doc->line(i);
            kate::IncludeParseResult r = parseIncludeDirective(line_str, false);
            if (r.m_range.isValid())
            {
                r.m_range.setBothLines(i);
                candidates.push_back(doc->text(r.m_range));
            }
        }
        // Resolve relative filenames to absolute
        QStringList all;
        for (const QString& file : candidates)
        {
            QStringList cfpl = findFileLocations(file);     // fill `Current File Paths List' ;-)
            /// \todo WTF! List doesn't have a \c merge() ???
            all.append(cfpl);
        }
        candidates.swap(all);
        QString error_text = filename.isEmpty()
          ? QString()
          : i18n("Unable to find the file: `<tt>%1</tt>'.", filename)
          + (candidates.isEmpty()
              ? QString()
              : i18n("<p>Here is a list of #included headers in the current file...</p>")
              )
          ;
        if (!error_text.isEmpty())
            KPassivePopup::message(
                i18n("Error")
              , "<qt>" + error_text + "</qt>"
              , qobject_cast<QWidget*>(this)
              );
    }
    if (!candidates.isEmpty())
        openFile(ChooseFromListDialog::selectHeaderToOpen(qobject_cast<QWidget*>(this), candidates));
}

QStringList IncludeHelperPluginView::findFileLocations(const QString& filename)
{
    KTextEditor::Document* doc = mainWindow()->activeView()->document();
    // Try to find full filename to open
    QStringList candidates = findHeader(filename, m_plugin->config().sessionDirs(), m_plugin->config().systemDirs());
    // Check CWD as well, if allowed
    if (m_plugin->config().useCwd())
    {
        const QString uri = doc->url().prettyUrl() + '/' + filename;
        if (isPresentAndReadable(uri))
            candidates.push_front(uri);                     // Push to front cuz more likely that user wants it
    }
    removeDuplicates(candidates);                           // Remove possible duplicates
    return candidates;
}

/**
 * \note This function assume that given file is really exists.
 */
inline void IncludeHelperPluginView::openFile(const QString& file)
{
    if (file.isEmpty()) return;                             // Nothing to do if no file specified
    kDebug() << "Going to open " << file;
    KTextEditor::Document* new_doc = m_plugin->application()->documentManager()->openUrl(file);
    QFileInfo fi(file);
    if (fi.isReadable())
    {
        kDebug() << "Is file " << file << " writeable? -- " << fi.isWritable();
        new_doc->setReadWrite(fi.isWritable());
        mainWindow()->activateView(new_doc);
    }
    else
    {
        KPassivePopup::message(
            i18n("Open error")
          , i18n("File %1 is not readable", file)
          , qobject_cast<QWidget*>(this)
          );
    }
}

inline void IncludeHelperPluginView::openFiles(const QStringList& files)
{
    for (const QString& file : files)
        openFile(file);
}

void IncludeHelperPluginView::copyInclude()
{
    KTextEditor::View* kv = mainWindow()->activeView();
    if (!kv)
    {
        kDebug() << "no KTextEditor::View";
        return;
    }
    const KUrl& uri = kv->document()->url().prettyUrl();
    QString current_dir = uri.directory();
    QString longest_matched;
    QChar open = m_plugin->config().useLtGt() ? '<' : '"';
    QChar close = m_plugin->config().useLtGt() ? '>' : '"';
    kDebug() << "Got document name: " << uri;
    // Try to match local (per session) dirs first
    for (const QString& dir : m_plugin->config().sessionDirs())
        if (current_dir.startsWith(dir) && longest_matched.length() < dir.length())
            longest_matched = dir;
    if (longest_matched.isEmpty())
    {
        open = '<';
        close = '>';
        // Try to match global dirs next
        for (const QString& dir : m_plugin->config().systemDirs())
            if (current_dir.startsWith(dir) && longest_matched.length() < dir.length())
                longest_matched = dir;
    }
    if (!longest_matched.isEmpty())
    {
        QString text;
        if (isCOrPPSource(kv->document()->mimeType()))
        {
            kDebug() << "current_dir=" << current_dir << ", lm=" << longest_matched;
            int count = longest_matched.size();
            for (; count < current_dir.size() && current_dir[count] == '/'; ++count) {}
            current_dir.remove(0, count);
            kDebug() << "current_dir=" << current_dir << ", lm=" << longest_matched;
            if (!current_dir.isEmpty() && !current_dir.endsWith('/'))
                current_dir += '/';
            text = QString("#include %1%2%3")
              .arg(open)
              .arg(current_dir + uri.fileName())
              .arg(close);
        }
        else
        {
            text = uri.prettyUrl();
        }
        QApplication::clipboard()->setText(text);
    }
}

void IncludeHelperPluginView::viewChanged()
{
    kDebug() << "update view?";
    KTextEditor::View* view = mainWindow()->activeView();
    if (!view)
    {
        kDebug() << "no KTextEditor::View -- leave `open header' action as is...";
        return;
    }
    const QString& mime_str = view->document()->mimeType();
    kDebug() << "Current document type: " << mime_str;
    const bool enable_open_header_action = isCOrPPSource(mime_str);
    m_open_header->setEnabled(enable_open_header_action);
    if (enable_open_header_action)
    {
        m_copy_include->setText(i18n("Copy #include to Clipboard"));
        // Update current view (possible some dirs/files has changed)
        m_plugin->updateDocumentInfo(view->document());
    }
    else m_copy_include->setText(i18n("Copy File URI to Clipboard"));
}

/**
 * \todo What if view/document will change mme type? (after save as... for example)?
 * Maybe better to check highlighting style? For example wen new document just created,
 * but highlighted as C++ the code below wouldn't work! Even more, after document gets
 * saved to disk completion still wouldn't work! That is definitely SUXX
 */
void IncludeHelperPluginView::viewCreated(KTextEditor::View* view)
{
    kDebug() << "view created";
    if (isCOrPPSource(view->document()->mimeType()))
    {
        KTextEditor::CodeCompletionInterface* cc_iface =
            qobject_cast<KTextEditor::CodeCompletionInterface*>(view);
        if (cc_iface)
        {
            kDebug() << "C/C++ source: register #include completer";
            // Enable #include completions
            cc_iface->registerCompletionModel(new IncludeHelperPluginCompletionModel(view, m_plugin));
            cc_iface->setAutomaticInvocationEnabled(true);

            kDebug() << "C/C++ source: register clang based code completer";
            // Enable semantic C++ code completions
            cc_iface->registerCompletionModel(new ClangCodeCompletionModel(view, m_plugin));
        }
    }
    // Scan document for #include...
    m_plugin->updateDocumentInfo(view->document());
    // Schedule updates after document gets reloaded
    connect(
        view->document()
      , SIGNAL(reloaded(KTextEditor::Document*))
      , m_plugin
      , SLOT(updateDocumentInfo(KTextEditor::Document*))
      );
    // Schedule updates after new text gets inserted
    connect(
        view->document()
      , SIGNAL(textInserted(KTextEditor::Document*, const KTextEditor::Range&))
      , m_plugin
      , SLOT(textInserted(KTextEditor::Document*, const KTextEditor::Range&))
      );
}

#if 0
/// \todo Do we really need this?
void IncludeHelperPluginView::textChanged(
    KTextEditor::Document* doc
    , const KTextEditor::Range& /*old_range*/
    , const QString& old_text
    , const KTextEditor::Range& new_range
    )
{
    kDebug() << doc << " change text: new=" << doc->text(new_range) << ", " << old_text;
}
#endif

KTextEditor::Range IncludeHelperPluginView::currentWord() const
{
    KTextEditor::Range result;                              // default range is empty
    KTextEditor::View* kv = mainWindow()->activeView();
    if (!kv)
    {
        kDebug() << "no KTextEditor::View";
        return result;
    }

    // Return selected text if any
    if (kv->selection())
        return kv->selectionRange();

    if (!kv->cursorPosition().isValid())
    {
        kDebug() << "cursor not valid!";
        return result;
    }

    // Obtain a line under cursor
    int line = kv->cursorPosition().line();
    QString line_str = kv->document()->line(line);

    // Check if current line starts w/ #include
    kate::IncludeParseResult r = parseIncludeDirective(line_str, false);
    if (r.m_range.isValid())
    {
        r.m_range.setBothLines(line);
        return r.m_range;
    }

    // No #include parsed... fallback to the default way:
    // just search a word under cursor...

    // Get cirsor line/column
    int col = kv->cursorPosition().column();

    int start = qMax(qMin(col, line_str.length() - 1), 0);
    int end = start;
    kDebug() << "Arghh... trying w/ word under cursor starting from " << start;

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

    return KTextEditor::Range(line, start, line, end);
}
//END IncludeHelperPluginView

//BEGIN ChooseFromListDialog
ChooseFromListDialog::ChooseFromListDialog(QWidget* parent)
  : KDialog(parent)
{
    setModal(true);
    setButtons(KDialog::Ok | KDialog::Cancel);
    showButtonSeparator(true);
    setCaption(i18n("Choose Header File to Open"));

    m_list = new KListWidget(this);
    setMainWidget(m_list);

    connect(m_list, SIGNAL(executed(QListWidgetItem*)), this, SLOT(accept()));
}

QString ChooseFromListDialog::selectHeaderToOpen(QWidget* parent, const QStringList& strings)
{
    KConfigGroup gcg(KGlobal::config(), "IncludeHelperChooserDialog");
    ChooseFromListDialog dialog(parent);
    dialog.m_list->addItems(strings);                       // append gien items to the list
    if (!strings.isEmpty())                                 // if strings list isn't empty
    {
        dialog.m_list->setCurrentRow(0);                    // select 1st row in the list
        dialog.m_list->setFocus(Qt::TabFocusReason);        // and give focus to it
    }
    dialog.restoreDialogSize(gcg);                          // restore dialog geometry from config

    QStringList result;
    if (dialog.exec() == QDialog::Accepted)                 // if user accept smth
    {
        // grab seleted items into a result list
        for (QListWidgetItem* item : dialog.m_list->selectedItems())
            result.append(item->text());
    }
    dialog.saveDialogSize(gcg);                             // write dialog geometry to config
    gcg.sync();
    if (result.empty())
        return QString();
    assert("The only file expected" && result.size() == 1u);
    return result[0];
}
//END ChooseFromListDialog
}                                                           // namespace kate
// kate: hl C++11/Qt4;
