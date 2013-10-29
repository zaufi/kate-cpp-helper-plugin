/**
 * \file
 *
 * \brief Class \c kate::CppHelperPluginView (implementation)
 *
 * \date Mon Feb  6 06:17:32 MSK 2012 -- Initial design
 */
/*
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
#include <src/clang/location.h>
#include <src/clang/to_string.h>
#include <src/cpp_helper_plugin.h>
#include <src/choose_from_list_dialog.h>
#include <src/clang_code_completion_model.h>
#include <src/document_info.h>
#include <src/document_proxy.h>
#include <src/include_helper_completion_model.h>
#include <src/utils.h>

// Standard includes
#include <kate/application.h>
#include <kate/documentmanager.h>
#include <kate/mainwindow.h>
#include <KDE/KActionCollection>
#include <KDE/KLocalizedString>
#include <KDE/KMenu>
#include <KDE/KPassivePopup>
#include <KDE/KStringHandler>
#include <KDE/KTextEditor/CodeCompletionInterface>
#include <KDE/KTextEditor/MovingInterface>
#include <KDE/KTextEditor/TextHintInterface>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QKeyEvent>
#include <QtCore/QFileInfo>
#include <QtCore/QDirIterator>
#include <QtGui/QToolTip>
#include <cassert>
#include <set>
#include <stack>

namespace kate { namespace {
const QStringList HDR_EXTENSIONS = QStringList() << "h" << "hh" << "hpp" << "hxx" << "H";
const QStringList SRC_EXTENSIONS = QStringList() << "c" << "cc" << "cpp" << "cxx" << "C" << "inl";
}                                                           // anonymous namespace

//BEGIN CppHelperPluginView
/**
 * \note Instances of this class created once per main window
 */
CppHelperPluginView::CppHelperPluginView(
    Kate::MainWindow* mw
  , const KComponentData& data
  , CppHelperPlugin* plugin
  )
  : Kate::PluginView(mw)
  , Kate::XMLGUIClient(data)
  , m_plugin(plugin)
  , m_open_header(actionCollection()->addAction("file_open_included_header"))
  , m_copy_include(actionCollection()->addAction("edit_copy_include"))
  , m_switch(actionCollection()->addAction("file_open_switch_iface_impl"))
  , m_tool_view(
        mw->createToolView(
            "kate_private_plugin_katecppplugin"
          , Kate::MainWindow::Bottom
          , SmallIcon("source-cpp11")
          , i18n("C++ Helper")
          )
      )
  , m_tool_view_interior(new Ui_PluginToolViewWidget())
  , m_includes_list_model(new QStandardItemModel())
  , m_last_explored_document(nullptr)
#if 0
  , m_menu(new KActionMenu(i18n("C++ Helper: Playground"), this))
#endif
{
    //BEGIN Setup plugin actions
    m_open_header->setText(i18n("Open Header Under Cursor"));
    m_open_header->setShortcut(QKeySequence(Qt::Key_F10));
    connect(m_open_header, SIGNAL(triggered(bool)), this, SLOT(openHeader()));

    m_switch->setText(i18n("Open Header/Implementation"));
    m_switch->setShortcut(QKeySequence(Qt::Key_F12));
    connect(m_switch, SIGNAL(triggered(bool)), this, SLOT(switchIfaceImpl()));

    m_copy_include->setText(i18n("Copy #include to Clipboard"));
    m_copy_include->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_F10));
    connect(m_copy_include, SIGNAL(triggered(bool)), this, SLOT(copyInclude()));

#if 0
    actionCollection()->addAction("popup_cpphelper", m_menu.get());
    m_what_is_this = m_menu->menu()->addAction(
        i18n("What is it at cursor?", QString())
      , this
      , SLOT(whatIsThis())
      );
    connect(m_menu->menu(), SIGNAL(aboutToShow()), this, SLOT(aboutToShow()));
#endif
    //END Setup plugin actions

    // On viewCreated we have to subscribe self to monitor 
    // some document properties and act correspondignly:
    // 0) on name/URL changes try to register/deregister
    connect(
        mainWindow()
      , SIGNAL(viewCreated(KTextEditor::View*))
      , this
      , SLOT(viewCreated(KTextEditor::View*))
      );

    // We want to enable/disable open header action depending on
    // mime-type of the current document, so we have to subscribe
    // to view changes...
    connect(mainWindow(), SIGNAL(viewChanged()), this, SLOT(updateCppActionsAvailability()));

    //BEGIN Setup toolview
    m_tool_view_interior->setupUi(new QWidget(m_tool_view.get()));
    m_tool_view->installEventFilter(this);

    // Diagnostic messages tab
    m_tool_view_interior->diagnosticMessages->setModel(&m_diagnostic_data);
    m_tool_view_interior->diagnosticMessages->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(
        m_tool_view_interior->diagnosticMessages
      , SIGNAL(activated(const QModelIndex&))
      , this
      , SLOT(diagnosticMessageActivated(const QModelIndex&))
      );
    {
        auto* clear_action = new QAction(
            KIcon("edit-clear-list")
          , i18n("Clear")
          , m_tool_view_interior->diagnosticMessages
          );
        m_tool_view_interior->diagnosticMessages->insertAction(nullptr, clear_action);
        connect(
            clear_action
          , SIGNAL(triggered(bool))
          , &m_diagnostic_data
          , SLOT(clear())
          );
    }

    // Search tab
    {
        auto model = m_plugin->databaseManager().getSearchResultsTableModel();
        m_tool_view_interior->searchResults->setModel(model);
        connect(
            model
          , SIGNAL(modelReset())
          , this
          , SLOT(searchResultsUpdated())
          );
    }
    connect(
        m_tool_view_interior->searchQuery
      , SIGNAL(returnPressed())
      , this
      , SLOT(startSearch())
      );

    // #include explorer tab
    m_tool_view_interior->includesTree->setHeaderHidden(true);
    m_tool_view_interior->includedFromList->setModel(m_includes_list_model);
    m_tool_view_interior->searchFilter->addTreeWidget(m_tool_view_interior->includesTree);
    connect(
        m_tool_view_interior->updateButton
      , SIGNAL(clicked())
      , this
      , SLOT(updateInclusionExplorer())
      );
    connect(
        m_tool_view_interior->includesTree
      , SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int))
      , this
      , SLOT(includeFileDblClickedFromTree(QTreeWidgetItem*, int))
      );
    connect(
        m_tool_view_interior->includesTree
      , SIGNAL(itemActivated(QTreeWidgetItem*, int))
      , this
      , SLOT(includeFileActivatedFromTree(QTreeWidgetItem*, int))
      );
    connect(
        m_tool_view_interior->includedFromList
      , SIGNAL(doubleClicked(const QModelIndex&))
      , this
      , SLOT(includeFileDblClickedFromList(const QModelIndex&))
      );

    // Indexer settings tab
    m_tool_view_interior->databases->setModel(m_plugin->databaseManager().getDatabasesTableModel());
    m_tool_view_interior->targets->setModel(m_plugin->databaseManager().getTargetsListModel());
    connect(
        m_tool_view_interior->newDatabase
      , SIGNAL(clicked())
      , &m_plugin->databaseManager()
      , SLOT(createNewIndex())
      );
    connect(
        m_tool_view_interior->deleteDatabase
      , SIGNAL(clicked())
      , &m_plugin->databaseManager()
      , SLOT(removeCurrentIndex())
      );
    connect(
        m_tool_view_interior->reindexDatabase
      , SIGNAL(clicked())
      , &m_plugin->databaseManager()
      , SLOT(rebuildCurrentIndex())
      );
    connect(
        m_tool_view_interior->stopIndexer
      , SIGNAL(clicked())
      , &m_plugin->databaseManager()
      , SLOT(stopIndexer())
      );
    connect(
        m_tool_view_interior->databases
      , SIGNAL(activated(const QModelIndex&))
      , &m_plugin->databaseManager()
      , SLOT(refreshCurrentTargets(const QModelIndex&))
      );
    connect(
        m_tool_view_interior->targets
      , SIGNAL(activated(const QModelIndex&))
      , &m_plugin->databaseManager()
      , SLOT(selectCurrentTarget(const QModelIndex&))
      );
    connect(
        m_tool_view_interior->addTarget
      , SIGNAL(clicked())
      , &m_plugin->databaseManager()
      , SLOT(addNewTarget())
      );
    connect(
        m_tool_view_interior->removeTarget
      , SIGNAL(clicked())
      , &m_plugin->databaseManager()
      , SLOT(removeCurrentTarget())
      );
    connect(
        &m_plugin->databaseManager()
      , SIGNAL(diagnosticMessage(DiagnosticMessagesModel::Record))
      , this
      , SLOT(addDiagnosticMessage(DiagnosticMessagesModel::Record))
      );
    connect(
        &m_plugin->databaseManager()
      , SIGNAL(reindexingStarted(const QString&))
      , this
      , SLOT(reindexingStarted(const QString&))
      );
    connect(
        &m_plugin->databaseManager()
      , SIGNAL(reindexingFinished(const QString&))
      , this
      , SLOT(reindexingFinished(const QString&))
      );


    mainWindow()->guiFactory()->addClient(this);
}

CppHelperPluginView::~CppHelperPluginView()
{
    mainWindow()->guiFactory()->removeClient(this);
}

void CppHelperPluginView::readSessionConfig(KConfigBase*, const QString& groupPrefix)
{
    kDebug(DEBUG_AREA) << "** VIEW **: Reading session config: " << groupPrefix;
}

void CppHelperPluginView::writeSessionConfig(KConfigBase*, const QString& groupPrefix)
{
    kDebug(DEBUG_AREA) << "** VIEW **: Writing session config: " << groupPrefix;
}

void CppHelperPluginView::addDiagnosticMessage(DiagnosticMessagesModel::Record record)
{
    m_diagnostic_data.append(std::move(record));
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
void CppHelperPluginView::switchIfaceImpl()
{
    assert(
        "Active view suppose to be valid at this point! Am I wrong?"
      && mainWindow()->activeView()
      );
    // Ok, trying case 1
    auto* doc = mainWindow()->activeView()->document();
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
                if (r.m_range.isValid())
                {
                    r.m_range.setBothLines(i);
                    first_header_name = doc->text(r.m_range);
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
                    QDirIterator dir_it {
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

/**
 * Action to open a header under cursor
 */
void CppHelperPluginView::openHeader()
{
    assert(
        "Active view suppose to be valid at this point! Am I wrong?"
      && mainWindow()->activeView()
      );

    QStringList candidates;
    QString filename;
    auto* doc = mainWindow()->activeView()->document();

    auto r = findIncludeFilenameNearCursor();
    kDebug(DEBUG_AREA) << "findIncludeFilenameNearCursor() = " << r;
    if (!r.isEmpty())
    {
        filename = doc->text(r).trimmed();
        assert("Getting text on non-empty range should return smth!" && !filename.isEmpty());

        // Try to find an absolute path to given filename
        candidates = findFileLocations(filename);
        kDebug(DEBUG_AREA) << "Found candidates: " << candidates;
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
            const auto& line_str = doc->line(i);
            auto r = parseIncludeDirective(line_str, false);
            if (r.m_range.isValid())
            {
                r.m_range.setBothLines(i);
                candidates.push_back(doc->text(r.m_range));
            }
        }
        // Resolve relative filenames to absolute
        QStringList all;
        for (const auto& file : candidates)
        {
            auto cfpl = findFileLocations(file);            // fill `Current File Paths List' ;-)
            /// \todo WTF! List doesn't have a \c merge() ???
            all.append(cfpl);
        }
        candidates.swap(all);
        auto error_text = filename.isEmpty()
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

QStringList CppHelperPluginView::findFileLocations(const QString& filename)
{
    assert(
        "Active view suppose to be valid at this point! Am I wrong?"
      && mainWindow()->activeView()
      );

    auto* doc = mainWindow()->activeView()->document();
    // Try to find full filename to open
    auto candidates = findHeader(filename, m_plugin->config().sessionDirs(), m_plugin->config().systemDirs());
    // Check CWD as well, if allowed
    if (m_plugin->config().useCwd())
    {
        const auto uri = QString{doc->url().prettyUrl() + '/' + filename};
        if (isPresentAndReadable(uri))
            candidates.push_front(uri);                     // Push to front cuz more likely that user wants it
    }
    removeDuplicates(candidates);                           // Remove possible duplicates
    return candidates;
}

/**
 * \note This function assume that given file is really exists.
 * \todo Turn the parameter into \c KUrl
 */
inline void CppHelperPluginView::openFile(const QString& file)
{
    if (file.isEmpty()) return;                             // Nothing to do if no file specified
    kDebug(DEBUG_AREA) << "Going to open " << file;
    auto* new_doc = m_plugin->application()->documentManager()->openUrl(file);
    auto fi = QFileInfo{file};
    if (fi.isReadable())
    {
        kDebug(DEBUG_AREA) << "Is file " << file << " writeable? -- " << fi.isWritable();
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

inline void CppHelperPluginView::openFiles(const QStringList& files)
{
    for (const auto& file : files)
        openFile(file);
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
 * \todo What if view/document will change mime type? (after save as... for example)?
 * Maybe better to check highlighting style? For example wen new document just created,
 * but highlighted as C++ the code below wouldn't work! Even more, after document gets
 * saved to disk completion still wouldn't work! That is definitely SUXX
 */
void CppHelperPluginView::viewCreated(KTextEditor::View* view)
{
    kDebug(DEBUG_AREA) << "view created";

    // Try to execute initial completers registration and #includes scanning
    if (handleView(view))
        m_plugin->updateDocumentInfo(view->document());

    auto* th_iface = qobject_cast<KTextEditor::TextHintInterface*>(view);
    if (th_iface)
    {
        connect(
            view
          , SIGNAL(needTextHint(const KTextEditor::Cursor&, QString&))
          , this
          , SLOT(needTextHint(const KTextEditor::Cursor&, QString&))
          );
        th_iface->enableTextHints(3000);                    // 3 seconds
    }

    auto* doc = view->document();
    // Rescan document for #includes on reload
    connect(
        doc
      , SIGNAL(reloaded(KTextEditor::Document*))
      , m_plugin
      , SLOT(updateDocumentInfo(KTextEditor::Document*))
      );
    // Scan inserted text for #includes if something were added to the document
    connect(
        doc
      , SIGNAL(textInserted(KTextEditor::Document*, const KTextEditor::Range&))
      , this
      , SLOT(textInserted(KTextEditor::Document*, const KTextEditor::Range&))
      );
    // Unnamed documents can change highlighting mode, so we have to register
    // completers and scan for #includes. For example if new document was created
    // from template.
    connect(
        doc
      , SIGNAL(highlightingModeChanged(KTextEditor::Document*))
      , this
      , SLOT(modeChanged(KTextEditor::Document*))
      );
    // If URL or name gets changed check if this is sill suitable document
    connect(
        doc
      , SIGNAL(documentNameChanged(KTextEditor::Document*))
      , this
      , SLOT(urlChanged(KTextEditor::Document*))
      );
    connect(
        doc
      , SIGNAL(documentUrlChanged(KTextEditor::Document*))
      , this
      , SLOT(urlChanged(KTextEditor::Document*))
      );
    connect(
        doc
      , SIGNAL(aboutToClose(KTextEditor::Document*))
      , this
      , SLOT(onDocumentClose(KTextEditor::Document*))
      );
    /// \note We don't need to subscribe to document close (connecting to
    /// \c Document::aboutToClose signal) to unregister completers cuz at this
    /// moment no views exists! I.e. iterating over \c view() gives no matches
    /// with stored in the \c m_completers map... :(
    /// Adding view as parent of completer we'll guarantied that on view destroy
    /// our completers will die as well. But we have to \c delete compilers
    /// on unregister...
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
        auto& di = m_plugin->getDocumentInfo(doc);
        auto status = di.getLineStatus(pos.line());
        switch (status)
        {
            case DocumentInfo::Status::NotFound:
                text = i18n("File not found");
                break;
            case DocumentInfo::Status::MultipleMatches:
                text = i18n("Multiple files matched");
                break;
            default:
                break;
        }
        QPoint p = view->cursorToCoordinate(pos);
        QToolTip::showText(p, text, view);
    }
}

void CppHelperPluginView::onDocumentClose(KTextEditor::Document* doc)
{
    if (doc == m_last_explored_document)
    {
        m_last_explored_document = nullptr;
        m_tool_view_interior->includesTree->clear();
        m_includes_list_model->clear();
    }
}

void CppHelperPluginView::modeChanged(KTextEditor::Document* doc)
{
    kDebug(DEBUG_AREA) << "hl mode has been changed: " << doc->highlightingMode() << ", " << doc->mimeType();
    if (handleView(doc->activeView()))
        m_plugin->updateDocumentInfo(doc);
}

void CppHelperPluginView::urlChanged(KTextEditor::Document* doc)
{
    kDebug(DEBUG_AREA) << "name or URL has been changed: " << doc->url() << ", " << doc->mimeType();
    if (handleView(doc->activeView()))
        m_plugin->updateDocumentInfo(doc);
}

/**
 * \todo Move to the plugin class?
 */
void CppHelperPluginView::textInserted(KTextEditor::Document* doc, const KTextEditor::Range& range)
{
    kDebug(DEBUG_AREA) << doc << " new text: " << doc->text(range);
    auto* mv_iface = qobject_cast<KTextEditor::MovingInterface*>(doc);
    if (!mv_iface)
    {
        kDebug(DEBUG_AREA) << "No moving iface!!!!!!!!!!!";
        return;
    }
    // Do we really need to scan this file?
    if (!isSuitableDocument(doc->mimeType(), doc->highlightingMode()))
    {
        kDebug(DEBUG_AREA) << "Document doesn't looks like C or C++: type ="
          << doc->mimeType() << ", hl =" << doc->highlightingMode();
        return;
    }
    // Find corresponding document info, insert if needed
    auto& di = m_plugin->getDocumentInfo(doc);
    // Search lines and filenames #include'd in this range
    /**
     * \todo It would be \b cool to have a view class over a document
     * so the following code would be possible:
     * \code
     *  for (const auto& l : DocumentLinesView(range, doc))
     *  {
     *      QString line_str = l;    // auto converstion to strings
     *      int line_no = l.index(); // tracking of the current line number
     *  }
     *  // Or even smth like this
     *  DocumentLinesView dv = { view->document() };
     *  // Get line text by number
     *  QString line_str = dv[line_no];
     * \endcode
     */
    for (auto i = range.start().line(); i < range.end().line() + 1; i++)
    {
        const auto& line_str = doc->line(i);
        kate::IncludeParseResult r = parseIncludeDirective(line_str, true);
        if (r.m_range.isValid())
        {
            r.m_range.setBothLines(i);
            if (!di.isRangeWithSameLineExists(r.m_range))
            {
                di.addRange(
                    mv_iface->newMovingRange(
                        r.m_range
                      , KTextEditor::MovingRange::ExpandLeft | KTextEditor::MovingRange::ExpandRight
                      )
                  );
            }
            else kDebug(DEBUG_AREA) << "range already registered";
        }
        else kDebug(DEBUG_AREA) << "no valid #include found";
    }
}

/**
 * Check if view has \c KTextEditor::CodeCompletionInterface and do nothing if it doesn't.
 * Otherwise, check if current document has suitable type (C/C++ source/header):
 * \li if yes, check if completers already registered, and register them if they don't
 * \li if not suitable document, check if any completers was registered for a given view
 * and unregister if it was.
 *
 * \param view view to register completers for. Do nothing if \c nullptr.
 * \return \c true if suitable document and registration successed, \c false otherwise.
 */
bool CppHelperPluginView::handleView(KTextEditor::View* view)
{
    if (!view)                                              // Null view is quite possible...
        return false;                                       // do nothing in this case.

    const auto is_suitable_document =
        isSuitableDocument(view->document()->mimeType(), view->document()->highlightingMode());

    updateCppActionsAvailability(is_suitable_document);

    auto* cc_iface = qobject_cast<KTextEditor::CodeCompletionInterface*>(view);
    if (!cc_iface)
    {
        kDebug(DEBUG_AREA) << "Nothing to do if no completion iface present for a view";
        return false;
    }
    auto result = false;
    auto it = m_completers.find(view);
    // Is current document has suitable type (or highlighting)
    if (is_suitable_document)
    {
        // Yeah! Check if still registration required
        if (it == end(m_completers))
        {
            kDebug(DEBUG_AREA) << "C/C++ source: register #include and code completers";
            std::unique_ptr<IncludeHelperCompletionModel> include_completer(
                new IncludeHelperCompletionModel(view, m_plugin)
              );
            std::unique_ptr<ClangCodeCompletionModel> code_completer(
                new ClangCodeCompletionModel(view, m_plugin, m_diagnostic_data)
              );
            auto r = m_completers.insert(
                std::make_pair(
                    view
                  , std::make_pair(include_completer.release(), code_completer.release())
                  )
              );
            assert("Completers expected to be new" && r.second);
            // Enable #include completions
            cc_iface->registerCompletionModel(r.first->second.first);
            // Enable semantic C++ code completions
            cc_iface->registerCompletionModel(r.first->second.second);
            // Turn auto completions ON
            cc_iface->setAutomaticInvocationEnabled(true);
            result = true;
        }
    }
    else
    {
        // Oops! Do not any completers required for this document.
        // Check if some was registered before, and erase if it is
        if (it != end(m_completers))
        {
            kDebug(DEBUG_AREA) << "Not a C/C++ source (anymore): unregister #include and code completers";
            cc_iface->unregisterCompletionModel(it->second.first);
            cc_iface->unregisterCompletionModel(it->second.second);
            /// \todo Is there any damn way to avoid explicit delete???
            /// Fraking Qt (as far as I know) was developed w/ automatic
            /// memory management (to help users not to think about it),
            /// so WHY I still have to call \c new and/or \c delete !!!
            /// FRAKING WHY!?
            delete it->second.first;
            delete it->second.second;
            m_completers.erase(it);
        }
    }
    kDebug(DEBUG_AREA) << "RESULT:" << result;
    return result;
}

bool CppHelperPluginView::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        auto* ke = static_cast<QKeyEvent*>(event);
        if ((obj == m_tool_view.get()) && (ke->key() == Qt::Key_Escape))
        {
            mainWindow()->hideToolView(m_tool_view.get());
            event->accept();
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

KTextEditor::Range CppHelperPluginView::findIncludeFilenameNearCursor() const
{
    assert(
        "Active view suppose to be valid at this point! Am I wrong?"
      && mainWindow()->activeView()
      );

    KTextEditor::Range result;                              // default range is empty
    auto* view = mainWindow()->activeView();
    if (!view || !view->cursorPosition().isValid())
        return result;                                      // do nothing if no view or valid cursor

    // Return selected text if any
    if (view->selection())
        return view->selectionRange();

    // Obtain a line under cursor
    const auto line = view->cursorPosition().line();
    const auto line_str = view->document()->line(line);

    // Check if current line starts w/ #include
    kate::IncludeParseResult r = parseIncludeDirective(line_str, false);
    if (r.m_range.isValid())
    {
        r.m_range.setBothLines(line);
        kDebug(DEBUG_AREA) << "Ok, found #include directive:" << r.m_range;
        return r.m_range;
    }

    // No #include parsed... fallback to the default way:
    // just search a word under cursor...

    // Get cursor line/column
    auto col = view->cursorPosition().column();

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

    return KTextEditor::Range(line, start, line, end);
}

#if 0
void CppHelperPluginView::aboutToShow()
{
    assert(
        "Active view suppose to be valid at this point! Am I wrong?"
      && mainWindow()->activeView()
      );

    KTextEditor::View* view = mainWindow()->activeView();
    if (view && view->cursorPosition().isValid())
    {
        DocumentProxy doc = mainWindow()->activeView()->document();
        auto range = doc.getIdentifierUnderCursor(view->cursorPosition());
        kDebug(DEBUG_AREA) << "current word range: " << range;
        if (range.isValid() && !range.isEmpty())
        {
            m_what_is_this->setEnabled(true);
            kDebug(DEBUG_AREA) << "current word text: " << doc->text(range);
            const QString squeezed = KStringHandler::csqueeze(doc->text(range), 30);
            m_what_is_this->setText(i18n("What is '%1'", squeezed));
            return;
        }
    }
    m_what_is_this->setEnabled(false);
    m_what_is_this->setText(i18n("What is ..."));
}

void CppHelperPluginView::whatIsThis()
{
    assert(
        "Active view suppose to be valid at this point! Am I wrong?"
      && mainWindow()->activeView()
      );

    KTextEditor::View* view = mainWindow()->activeView();
    if (!view || !view->cursorPosition().isValid())
        return;                                             // do nothing if no view or valid cursor

#if 0
    // view is of type KTextEditor::View*
    auto* iface = qobject_cast<KTextEditor::AnnotationViewInterface*>(view);
    if (iface)
    {
        iface->setAnnotationBorderVisible(!iface->isAnnotationBorderVisible());
    }
#endif

#if 0
    QByteArray filename = view->document()->url().toLocalFile().toAscii();
    auto& unit = m_plugin->getTranslationUnitByDocument(view->document());
    CXFile file = clang_getFile(unit, filename.constData());
    CXSourceLocation loc = clang_getLocation(
        unit
      , file
      , view->cursorPosition().line() + 1
      , view->cursorPosition().column() + 1
      );
    CXCursor ctx = clang_getCursor(unit, loc);
    kDebug(DEBUG_AREA) << "Cursor: " << ctx;
    CXCursor spctx = clang_getCursorSemanticParent(ctx);
    kDebug(DEBUG_AREA) << "Cursor of semantic parent: " << spctx;
    CXCursor lpctx = clang_getCursorLexicalParent(ctx);
    kDebug(DEBUG_AREA) << "Cursor of lexical parent: " << lpctx;

    clang::DCXString comment = clang_Cursor_getRawCommentText(ctx);
    kDebug(DEBUG_AREA) << "Associated comment:" << clang_getCString(comment);

    clang::DCXString usr = clang_getCursorUSR(ctx);
    kDebug(DEBUG_AREA) << "USR:" << clang_getCString(usr);
#endif
}
#endif                                                      // 0

namespace details {
struct InclusionVisitorData
{
    CppHelperPluginView* const m_self;
    DocumentInfo* m_di;
    std::stack<QTreeWidgetItem*> m_parents;
    std::set<int> m_visited_ids;
    QTreeWidgetItem* m_last_added_item;
    unsigned m_last_stack_size;
};
}                                                           // namespace details

void CppHelperPluginView::updateInclusionExplorer()
{
    assert(
        "Active view suppose to be valid at this point! Am I wrong?"
      && mainWindow()->activeView()
      );

    // Show busy mouse pointer
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

    auto* doc = mainWindow()->activeView()->document();
    auto& unit = m_plugin->getTranslationUnitByDocument(doc, false);
    // Obtain diagnostic if any
    {
        auto diag = unit.getLastDiagnostic();
        m_diagnostic_data.append(
            std::make_move_iterator(begin(diag))
          , std::make_move_iterator(end(diag))
          );
    }
    details::InclusionVisitorData data = {this, &m_plugin->getDocumentInfo(doc), {}, {}, nullptr, 0};
    data.m_di->clearInclusionTree();                        // Clear a previous tree in the document info
    m_tool_view_interior->includesTree->clear();            // and in the tree view model
    m_includes_list_model->clear();                         // as well as `included by` list
    data.m_parents.push(m_tool_view_interior->includesTree->invisibleRootItem());
    clang_getInclusions(
        unit
      , [](
            CXFile file
          , CXSourceLocation* stack
          , unsigned stack_size
          , CXClientData d
          )
        {
            auto* user_data = static_cast<details::InclusionVisitorData* const>(d);
            user_data->m_self->inclusionVisitor(user_data, file, stack, stack_size);
        }
      , &data
      );
    m_last_explored_document = doc;                         // Remember the document last explored

    QApplication::restoreOverrideCursor();                  // Restore mouse pointer to normal
    kDebug(DEBUG_AREA) << "headers cache now has" << m_plugin->headersCache().size() << "items!";
}

void CppHelperPluginView::inclusionVisitor(
    details::InclusionVisitorData* data
  , CXFile file
  , CXSourceLocation* stack
  , unsigned stack_size
  )
{
    const auto header_name = clang::toString(clang::DCXString{clang_getFileName(file)});
    // Obtain (or assign a new) an unique header ID
    const auto header_id = m_plugin->headersCache()[header_name];

    auto included_from_id = DocumentInfo::IncludeLocationData::ROOT;
    clang::location loc;
    if (stack_size)                                         // Is there anything on stack
    {
        loc = {stack[0]};                                   // NOTE Take filename from top of stack only!
        // Obtain a filename and its ID in headers cache
        included_from_id = m_plugin->headersCache()[loc.file().toLocalFile()];
    }
    // NOTE Kate has zero based cursor position, so -1 is here
    data->m_di->addInclusionEntry({header_id, included_from_id, loc.line() - 1, loc.column() - 1});

    QTreeWidgetItem* parent = nullptr;
    if (data->m_last_stack_size < stack_size)               // Is current stack grew relative to the last call
    {
        assert("Sanity check!" && (stack_size - data->m_last_stack_size == 1));
        // We have to add one more parent
        data->m_parents.push(data->m_last_added_item);
        parent = data->m_last_added_item;
    }
    else if (stack_size < data->m_last_stack_size)
    {
        // Stack size reduced since tha last call: remove our top
        for (unsigned i = data->m_last_stack_size; i > stack_size; --i)
            data->m_parents.pop();
        assert("Stack expected to be non empty!" && !data->m_parents.empty());
        parent = data->m_parents.top();
    }
    else
    {
        assert("Sanity check!" && stack_size == data->m_last_stack_size);
        parent = data->m_parents.top();
    }
    assert("Sanity check!" && parent);
    data->m_last_added_item = new QTreeWidgetItem(parent);
    data->m_last_added_item->setText(0, header_name);
    auto it = data->m_visited_ids.find(header_id);
    if (it != end(data->m_visited_ids))
        data->m_last_added_item->setForeground(0, Qt::yellow);
    else
        data->m_visited_ids.insert(header_id);
    data->m_last_stack_size = stack_size;
}

void CppHelperPluginView::includeFileActivatedFromTree(QTreeWidgetItem* item, const int column)
{
    assert("Document expected to be alive" && m_last_explored_document);

    m_includes_list_model->clear();

    const auto& cache = const_cast<const CppHelperPlugin* const>(m_plugin)->headersCache();
    auto filename = item->data(column, Qt::DisplayRole).toString();
    auto id = cache[filename];
    if (id != HeaderFilesCache::NOT_FOUND)
    {
        const auto& di = m_plugin->getDocumentInfo(m_last_explored_document);
        auto included_from = di.getListOfIncludedBy2(id);
        for (const auto& entry : included_from)
        {
            if (entry.m_included_by_id != DocumentInfo::IncludeLocationData::ROOT)
            {
                auto* item = new QStandardItem(
                    QString("%1[%2]").arg(
                        cache[entry.m_included_by_id]
                      , QString::number(entry.m_line)
                      )
                  );
                m_includes_list_model->appendRow(item);
            }
        }
    }
    else
    {
        kDebug(DEBUG_AREA) << "WTF: " << filename << "NOT FOUND!?";
    }
}

void CppHelperPluginView::dblClickOpenFile(QString&& filename)
{
    assert("Filename expected to be non empty" && !filename.isEmpty());
    openFile(filename);
}

void CppHelperPluginView::includeFileDblClickedFromTree(QTreeWidgetItem* item, const int column)
{
    dblClickOpenFile(item->data(column, Qt::DisplayRole).toString());
}

void CppHelperPluginView::includeFileDblClickedFromList(const QModelIndex& index)
{
    assert(
        "Active view suppose to be valid at this point! Am I wrong?"
      && mainWindow()->activeView()
      );

    auto filename = m_includes_list_model->itemFromIndex(index)->text();
    auto pos = filename.lastIndexOf('[');
    auto line = filename.mid(pos + 1, filename.lastIndexOf(']') - pos - 1).toInt(nullptr);
    filename.remove(pos, filename.size());
    dblClickOpenFile(std::move(filename));
    mainWindow()->activeView()->setCursorPosition({line, 0});
    mainWindow()->activeView()->setSelection({line, 0, line + 1, 0});
}

void CppHelperPluginView::diagnosticMessageActivated(const QModelIndex& index)
{
    auto loc = m_diagnostic_data.getLocationByIndex(index);
    if (!loc.empty())
    {
        const auto& filename = loc.file().toLocalFile();
        assert("Filename expected to be non empty!" && !filename.isEmpty());
        openFile(filename);
        // NOTE Kate has line/column numbers started from 0, but clang is more
        // human readable...
        mainWindow()->activeView()->setCursorPosition({loc.line() - 1, loc.column() - 1});
#if 0
        /// \todo Make it configurable?
        mainWindow()->activeView()->setFocus(Qt::MouseFocusReason);
#endif
    }
}

/**
 * Enable/disable C++ specific actions (copy \c #include, update button in explorer, etc.)
 */
void CppHelperPluginView::updateCppActionsAvailability()
{
    auto* view = mainWindow()->activeView();
    if (!view)
    {
        kDebug(DEBUG_AREA) << "no active view yet -- leave `open header' action as is...";
        return;
    }
    const auto& mime = view->document()->mimeType();
    const auto& hl = view->document()->highlightingMode();
    const bool is_ok = isSuitableDocument(mime, hl);
    kDebug(DEBUG_AREA) << "MIME:" << mime << ", HL:" << hl << " --> " << (is_ok ? "Enable" : "Disable");
    updateCppActionsAvailability(is_ok);
}

void CppHelperPluginView::updateCppActionsAvailability(const bool enable_cpp_specific_actions)
{
    kDebug(DEBUG_AREA) << "Enable C++ specific actions:" << enable_cpp_specific_actions;
    m_open_header->setEnabled(enable_cpp_specific_actions);
    m_tool_view_interior->updateButton->setEnabled(enable_cpp_specific_actions);
    if (enable_cpp_specific_actions)
        m_copy_include->setText(i18n("Copy #include to Clipboard"));
    else
        m_copy_include->setText(i18n("Copy File URI to Clipboard"));
}

void CppHelperPluginView::reindexingStarted(const QString& msg)
{
    addDiagnosticMessage(
        DiagnosticMessagesModel::Record{msg, DiagnosticMessagesModel::Record::type::info}
      );
    // Disable rebuild index button
    m_tool_view_interior->reindexDatabase->setEnabled(false);
    m_tool_view_interior->stopIndexer->setEnabled(true);
}

void CppHelperPluginView::reindexingFinished(const QString& msg)
{
    addDiagnosticMessage(
        DiagnosticMessagesModel::Record{msg, DiagnosticMessagesModel::Record::type::info}
      );
    // Enable rebuild index button
    m_tool_view_interior->reindexDatabase->setEnabled(true);
    m_tool_view_interior->stopIndexer->setEnabled(false);
}

void CppHelperPluginView::startSearch()
{
    auto query = m_tool_view_interior->searchQuery->text();
    kDebug() << "Search query: " << query;
    if (!query.isEmpty())
        m_plugin->databaseManager().startSearch(query);
}

void CppHelperPluginView::searchResultsUpdated()
{
    m_tool_view_interior->searchResults->setVisible(false);
    m_tool_view_interior->searchResults->resizeColumnsToContents();
    m_tool_view_interior->searchResults->setVisible(true);
}

//END CppHelperPluginView
}                                                           // namespace kate
// kate: hl C++11/Qt4;
