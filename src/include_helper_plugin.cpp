/**
 * \file
 *
 * \brief Class \c IncludeHelperPlugin (implementation)
 *
 * \date Sun Jan 29 09:15:53 MSK 2012 -- Initial design
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
#include <src/include_helper_plugin.h>

// Standard includes
#include <KAboutData>
#include <KActionCollection>
#include <KDebug>
#include <KDirSelectDialog>
#include <KPassivePopup>
#include <KPluginFactory>
#include <KPluginLoader>
#include <KTextEditor/Document>
#include <KTextEditor/View>
#include <QtCore/QFileInfo>
#include <QtGui/QClipboard>

namespace {
inline int area()
{
    static int s_area = KDebug::registerArea("include-helper", true);
    return s_area;
}
/// \c true if given MIME type string belongs to C/C++ source
inline bool isCOrPPSource(const QString& mime_str)
{
    return (mime_str == "text/x-c++src")
      || (mime_str == "text/x-c++hdr")
      || (mime_str == "text/x-csrc")
      || (mime_str == "text/x-chdr")
      ;
}
}                                                           // anonymous namespace


K_PLUGIN_FACTORY(IncludeHelperPluginFactory, registerPlugin<IncludeHelperPlugin>();)
K_EXPORT_PLUGIN(
    IncludeHelperPluginFactory(
        KAboutData(
            "kateincludehelperplugin"
          , "kate_includehelper_plugin"
          , ki18n("Include Helper Plugin")
          , "0.1"
          , ki18n("Helps to work w/ C/C++ headers little more easy")
          , KAboutData::License_LGPL_V3
          )
      )
  )

//BEGIN IncludeHelperPlugin
IncludeHelperPlugin::IncludeHelperPlugin(
    QObject* application
  , const QList<QVariant>&
  )
  : Kate::Plugin(static_cast<Kate::Application*>(application), "kate_includehelper_plugin")
{
}

Kate::PluginView* IncludeHelperPlugin::createView(Kate::MainWindow* parent)
{
    return new IncludeHelperPluginView(parent, IncludeHelperPluginFactory::componentData(), this);
}

Kate::PluginConfigPage* IncludeHelperPlugin::configPage(uint number, QWidget* parent, const char*)
{
    assert("This plugin have the only configuration page" && number == 0);
    if (number != 0)
        return 0;
    return new IncludeHelperPluginGlobalConfigPage(parent, this);
}

void IncludeHelperPlugin::readSessionConfig(KConfigBase* config, const QString& groupPrefix)
{
    // Read session config
    KConfigGroup scg(config, groupPrefix + ":include-helper");
    QStringList session_dirs = scg.readPathEntry("ConfiguredDirs", QStringList());
    kDebug() << "Got per session configured include path list: " << session_dirs;
    // Read global config
    KConfigGroup gcg(KGlobal::config(), "IncludeHelper");
    QStringList dirs = gcg.readPathEntry("ConfiguredDirs", QStringList());
    kDebug() << "Got global configured include path list: " << dirs;
    QVariant use_ltgt = gcg.readEntry("UseLtGt", QVariant(false));
    // Assign configuration
    m_session_dirs = session_dirs;
    m_global_dirs = dirs;
    m_use_ltgt = use_ltgt.toBool();
}

void IncludeHelperPlugin::writeSessionConfig(KConfigBase* config, const QString& groupPrefix)
{
    // Write session config
    kDebug() << "Write per session configured include path list: " << m_session_dirs;
    KConfigGroup scg(config, groupPrefix + ":include-helper");
    scg.writePathEntry("ConfiguredDirs", m_session_dirs);
    scg.sync();
    // Read global config
    kDebug() << "Write global configured include path list: " << m_global_dirs;
    KConfigGroup gcg(KGlobal::config(), "IncludeHelper");
    gcg.writePathEntry("ConfiguredDirs", m_global_dirs);
    gcg.writeEntry("UseLtGt", QVariant(m_use_ltgt));
    gcg.sync();
}

//END IncludeHelperPlugin

//BEGIN IncludeHelperPluginGlobalConfigPage
IncludeHelperPluginGlobalConfigPage::IncludeHelperPluginGlobalConfigPage(
    QWidget* parent
  , IncludeHelperPlugin* plugin
  )
  : Kate::PluginConfigPage(parent)
  , m_plugin(plugin)
{
    m_configuration_ui.setupUi(this);
    m_configuration_ui.addToGlobalButton->setIcon(KIcon("list-add"));
    m_configuration_ui.delFromGlobalButton->setIcon(KIcon("list-remove"));
    m_configuration_ui.addToSessionButton->setIcon(KIcon("list-add"));
    m_configuration_ui.delFromSessionButton->setIcon(KIcon("list-remove"));

    // Connect add/del buttons to actions
    connect(m_configuration_ui.addToGlobalButton, SIGNAL(clicked()), this, SLOT(addGlobalIncludeDir()));
    connect(m_configuration_ui.delFromGlobalButton, SIGNAL(clicked()), this, SLOT(delGlobalIncludeDir()));
    connect(m_configuration_ui.addToSessionButton, SIGNAL(clicked()), this, SLOT(addSessionIncludeDir()));
    connect(m_configuration_ui.delFromSessionButton, SIGNAL(clicked()), this, SLOT(delSessionIncludeDir()));

    // Populate configuration w/ dirs
    reset();
}

void IncludeHelperPluginGlobalConfigPage::apply()
{
    // Notify about configuration changes
    {
        QStringList dirs;
        for (int i = 0; i < m_configuration_ui.sessionDirsList->count(); ++i)
        {
            dirs.append(m_configuration_ui.sessionDirsList->item(i)->text());
        }
        m_plugin->setSessionDirs(dirs);
    }
    {
        QStringList dirs;
        for (int i = 0; i < m_configuration_ui.globalDirsList->count(); ++i)
        {
            dirs.append(m_configuration_ui.globalDirsList->item(i)->text());
        }
        m_plugin->setGlobalDirs(dirs);
    }
    m_plugin->setUseLtGt(m_configuration_ui.includeMarkersSwitch->checkState() == Qt::Checked);
}

void IncludeHelperPluginGlobalConfigPage::reset()
{
    kDebug() << "Reseting configuration";
    // Put dirs to the list
    m_configuration_ui.globalDirsList->addItems(m_plugin->globalDirs());
    m_configuration_ui.sessionDirsList->addItems(m_plugin->sessionDirs());
    m_configuration_ui.includeMarkersSwitch->setCheckState(
        m_plugin->useLtGt() ? Qt::Checked : Qt::Unchecked
      );
}

void IncludeHelperPluginGlobalConfigPage::addSessionIncludeDir()
{
    KUrl dir_uri = KDirSelectDialog::selectDirectory(KUrl(), true, this);
    if (dir_uri != KUrl())
    {
        const QString& dir_str = dir_uri.toLocalFile();
        if (!contains(dir_str, m_configuration_ui.sessionDirsList))
            new QListWidgetItem(dir_str, m_configuration_ui.sessionDirsList);
    }
}

void IncludeHelperPluginGlobalConfigPage::delSessionIncludeDir()
{
    delete m_configuration_ui.sessionDirsList->currentItem();
}

void IncludeHelperPluginGlobalConfigPage::addGlobalIncludeDir()
{
    KUrl dir_uri = KDirSelectDialog::selectDirectory(KUrl(), true, this);
    if (dir_uri != KUrl())
    {
        const QString& dir_str = dir_uri.toLocalFile();
        if (!contains(dir_str, m_configuration_ui.globalDirsList))
            new QListWidgetItem(dir_str, m_configuration_ui.globalDirsList);
    }
}

void IncludeHelperPluginGlobalConfigPage::delGlobalIncludeDir()
{
    delete m_configuration_ui.globalDirsList->currentItem();
}

bool IncludeHelperPluginGlobalConfigPage::contains(const QString& dir, const KListWidget* list)
{
    for (int i = 0; i < list->count(); ++i)
        if (list->item(i)->text() == dir)
            return true;
    return false;
}
//END IncludeHelperPluginGlobalConfigPage

//BEGIN IncludeHelperPluginView

IncludeHelperPluginView::IncludeHelperPluginView(Kate::MainWindow* mw, const KComponentData& data, IncludeHelperPlugin* plugin)
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

    // We want to enable/disable open header action depending on
    // mime-type of the current document, so we have to subscribe
    // to view changes...
    connect(mainWindow(), SIGNAL(viewChanged()), this, SLOT(viewChanged()));
    mainWindow()->guiFactory()->addClient(this);
}

IncludeHelperPluginView::~IncludeHelperPluginView() {
    mainWindow()->guiFactory()->removeClient(this);
}

void IncludeHelperPluginView::openHeader()
{
    KTextEditor::Range r = currentWord();
    kDebug() << "currentWord() = " << r;
    if (r.isEmpty())
        return;                                             // Nothing todo if range is empty
    // NOTE Non empty range means that main window is valid
    // and active view present as well!
    assert("Main window and active view expected to be valid!" && mainWindow()->activeView());
    QString filename = mainWindow()->activeView()->document()->text(r);
    assert("Getting text on non-empty range should return smth!" && !filename.isEmpty());
    kDebug() << "filename = " << filename;

    QStringList candidates;
    // Try to find full filename to open
    /// \todo Is there any way to make a joint view for both containers?
    // 0) search session dirs list first
    Q_FOREACH(const QString& dir, m_plugin->sessionDirs())
    {
        const QString uri = dir + "/" + filename;
        kDebug() << "Trying " << uri;
        const QFileInfo fi = QFileInfo(uri);
        if (fi.exists() && fi.isFile() && fi.isReadable())
            candidates.append(uri);
    }
    // 1) search global dirs next
    Q_FOREACH(const QString& dir, m_plugin->globalDirs())
    {
        const QString uri = dir + "/" + filename;
        kDebug() << "Trying " << (dir + "/" + filename);
        const QFileInfo fi = QFileInfo(uri);
        if (fi.exists() && fi.isFile() && fi.isReadable())
            candidates.append(uri);
    }
    //
    kDebug() << "Found candidates: " << candidates;

    // If there is no ambiguity, then just emit a signal to open the file
    if (candidates.size() == 1)
        mainWindow()->openUrl(candidates.first());
    else if (candidates.isEmpty())
        KPassivePopup::message(i18n("Header not found"), filename, qobject_cast<QWidget*>(this));
    else
    {
        QStringList selected = ChooseFromListDialog::select(qobject_cast<QWidget*>(this), candidates);
        Q_FOREACH(const QString& dir, selected)
            mainWindow()->openUrl(dir);
    }
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
        Q_FOREACH(const QString& dir, m_plugin->globalDirs())
            if (current_dir.startsWith(dir) && longest_matched.length() < dir.length())
                longest_matched = dir;
    }
    if (!longest_matched.isEmpty())
    {
        const QString filename = current_dir.remove(0, longest_matched.length()) + '/' + uri.fileName();
        kDebug() << "filename = " << filename;
        const QString& mime_str = kv->document()->mimeType();
        if (isCOrPPSource(mime_str))
            QApplication::clipboard()->setText(QString("#include %1%2%3").arg(open).arg(filename).arg(close));
        else
            QApplication::clipboard()->setText(uri.prettyUrl());
    }
}

void IncludeHelperPluginView::viewChanged()
{
    KTextEditor::View* kv = mainWindow()->activeView();
    if (!kv)
    {
        kDebug() << "no KTextEditor::View -- leave `open header' action as is...";
        return;
    }
    const QString& mime_str = kv->document()->mimeType();
    kDebug() << "Current document type: " << mime_str;
    const bool enable_open_header_action = isCOrPPSource(mime_str);
    m_open_header->setEnabled(enable_open_header_action);
    if (enable_open_header_action)
        m_copy_include->setText(i18n("Copy #include to Clipboard"));
    else
        m_copy_include->setText(i18n("Copy File URI to Clipboard"));
}

/**
 * Return a range w/ filename if given line contains a valid \c #include
 * directive, empty range otherwise.
 *
 * \param line a string w/ line to parse
 * \param strict return failure if closing \c '>' or \c '"' missed
 *
 * \return a range w/ filename of \c #include directive
 */
KTextEditor::Range IncludeHelperPluginView::parseIncludeDirective(const QString& line, const bool strict) const
{
    KTextEditor::Range result;
    enum State {
        skipInitialSpaces
      , foundHash
      , checkInclude
      , skipSpaces
      , foundOpenChar
      , findCloseChar
      , stop
    };
    int start = 0;
    int end = 0;
    int tmp = 0;
    QChar close = 0;
    State state = skipInitialSpaces;
    for (int pos = 0; pos < line.length() && state != stop; ++pos)
    {
        switch (state)
        {
            case skipInitialSpaces:
                if (!line[pos].isSpace())
                {
                    if (line[pos] == '#')
                    {
                        state = foundHash;
                        continue;
                    }
                    return result;
                }
                break;
            case foundHash:
                if (line[pos].isSpace())
                    continue;
                else
                    state = checkInclude;
                // NOTE No `break' here!
            case checkInclude:
                if ("include"[tmp++] != line[pos])
                    return result;
                if (tmp == 7)
                    state = skipSpaces;
                break;
            case skipSpaces:
                if (line[pos].isSpace())
                    continue;
                close = (line[pos] == '<') ? '>' : (line[pos] == '"') ? '"' : 0;
                if (close == 0)
                    return result;
                state = foundOpenChar;
                break;
            case foundOpenChar:
                state = findCloseChar;
                start = pos;
                end = pos;
                // NOTE No `break' here!
            case findCloseChar:
                if (line[pos] == close)
                {
                    end = pos;
                    state = stop;
                }
                else
                {
                    if (line[pos].isSpace())
                    {
                        if (strict)
                            return result;
                        end = pos;
                        state = stop;
                    }
                }
                break;
            case stop:
            default:
                assert(!"Parsing FSM broken!");
        }
    }
    return KTextEditor::Range(0, start, 0, end);
}

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
    KTextEditor::Range r = parseIncludeDirective(line_str, false);
    if (!r.isEmpty())
    {
        r.setBothLines(line);
        return r;
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
    // Seeking for end of current word
    while (end < line_str.length() && !line_str[end].isSpace() && line_str[end] != '>' && line_str[end] != '"')
    {
        ++end;
    }

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

QStringList ChooseFromListDialog::select(QWidget* parent, const QStringList& strings)
{
    KConfigGroup gcg(KGlobal::config(), "IncludeHelperChooserDialog");
    ChooseFromListDialog dialog(parent);
    dialog.m_list->addItems(strings);                       // append gien items to the list
    dialog.restoreDialogSize(gcg);                          // restore dialog geometry from config

    QStringList result;
    if (dialog.exec() == QDialog::Accepted)
    {
        Q_FOREACH(QListWidgetItem* item, dialog.m_list->selectedItems())
        {
            result.append(item->text());
        }
    }
    dialog.saveDialogSize(gcg);                             // write dialog geometry to config
    gcg.sync();
    return result;
}
//END ChooseFromListDialog
