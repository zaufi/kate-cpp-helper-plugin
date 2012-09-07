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
#include <cassert>

namespace kate {
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
{
    m_open_header->setText(i18n("Open Header Under Cursor"));
    m_open_header->setShortcut(QKeySequence(Qt::Key_F10));
    connect(m_open_header, SIGNAL(triggered(bool)), this, SLOT(openHeader()));

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

        filename = doc->text(r);
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
        if (!filename.isEmpty())
            KPassivePopup::message(
                i18n("Error")
              , i18n(
                    "<qt>Unable to find a file: `<tt>%1</tt>'."
                    "<p>Here is the list of #included headers...</p><qt>"
                  , filename
                  )
              , qobject_cast<QWidget*>(this)
              );
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
        Q_FOREACH(const QString& file, candidates)
        {
            QStringList cfpl = findFileLocations(file);     // fill `Current File Paths List' ;-)
            /// \todo WTF! List doesn't have a \c merge() ???
            all.append(cfpl);
        }
        candidates.swap(all);
    }
    openFiles(ChooseFromListDialog::selectHeaderToOpen(qobject_cast<QWidget*>(this), candidates));
}

QStringList IncludeHelperPluginView::findFileLocations(const QString& filename)
{
    KTextEditor::Document* doc = mainWindow()->activeView()->document();
    // Try to find full filename to open
    QStringList candidates = findHeader(filename, m_plugin->sessionDirs(), m_plugin->systemDirs());
    // Check CWD as well, if allowed
    if (m_plugin->useCwd())
    {
        const QString uri = doc->url().prettyUrl() + '/' + filename;
        if (isPresentAndReadable(uri))
            candidates.push_front(uri);                     // Push to front cuz more likely that user wants it
    }
    removeDuplicates(candidates);                           // Remove possible duplicates
    return candidates;
}

inline void IncludeHelperPluginView::openFile(const QString& file)
{
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
    Q_FOREACH(const QString& file, files)
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
    QChar open = m_plugin->useLtGt() ? '<' : '"';
    QChar close = m_plugin->useLtGt() ? '>' : '"';
    kDebug() << "Got document name: " << uri;
    // Try to match local (per session) dirs first
    Q_FOREACH(const QString& dir, m_plugin->sessionDirs())
        if (current_dir.startsWith(dir) && longest_matched.length() < dir.length())
            longest_matched = dir;
    if (longest_matched.isEmpty())
    {
        open = '<';
        close = '>';
        // Try to match global dirs next
        Q_FOREACH(const QString& dir, m_plugin->systemDirs())
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

void IncludeHelperPluginView::viewCreated(KTextEditor::View* view)
{
    kDebug() << "view created";
    if (isCOrPPSource(view->document()->mimeType()))
    {
        kDebug() << "C/C++ source: register #include completer";
        KTextEditor::CodeCompletionInterface* cc_iface =
            qobject_cast<KTextEditor::CodeCompletionInterface*>(view);
        if (cc_iface)
        {
            // Enable completions
            cc_iface->registerCompletionModel(new IncludeHelperPluginCompletionModel(view, m_plugin));
            cc_iface->setAutomaticInvocationEnabled(true);
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

QStringList ChooseFromListDialog::selectHeaderToOpen(QWidget* parent, const QStringList& strings)
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
        Q_FOREACH(QListWidgetItem* item, dialog.m_list->selectedItems())
            result.append(item->text());
    }
    dialog.saveDialogSize(gcg);                             // write dialog geometry to config
    gcg.sync();
    return result;
}
//END ChooseFromListDialog
}                                                           // namespace kate
