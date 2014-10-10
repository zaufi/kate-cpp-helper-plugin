/**
 * \file
 *
 * \brief Class \c kate::CppHelperPluginView (implementation part I)
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
#include <src/cpp_helper_plugin.h>
#include <src/clang_code_completion_model.h>
#include <src/include_helper_completion_model.h>
#include <src/preprocessor_completion_model.h>
#include <src/utils.h>

// Standard includes
#include <kate/mainwindow.h>
#include <KDE/KActionCollection>
#include <KDE/KActionMenu>
#include <KDE/KPassivePopup>
#include <KDE/KStringHandler>
#include <KDE/KTextEditor/CodeCompletionInterface>
#include <KDE/KTextEditor/MovingInterface>
#include <KDE/KTextEditor/TextHintInterface>
#include <QtGui/QKeyEvent>
#include <QtGui/QMenu>
#include <QtGui/QSortFilterProxyModel>
#include <QtGui/QStandardItemModel>
#include <cassert>

namespace kate { namespace {
const QString EXPLORER_TREE_WIDTH_KEY = "ExplorerTreeWidth";
const QString SEARCH_PANE_WIDTH_KEY = "SearchDetailsPaneWidth";
const QString INDICES_LIST_WIDTH_KEY = "IndicesListWidth";
const QString TOOL_VIEW_TABS_ORDER_KEY = "ToolViewTabsOrder";
}                                                           // anonymous namespace

//BEGIN CppHelperPluginView
/**
 * \note Instances of this class created once per main window
 */
CppHelperPluginView::CppHelperPluginView(
    Kate::MainWindow* const mw
  , const KComponentData& data
  , CppHelperPlugin* const plugin
  )
  : Kate::PluginView{mw}
  , Kate::XMLGUIClient{data}
  , m_plugin{plugin}
  , m_tool_view{
        mw->createToolView(
            plugin
          , "kate_private_plugin_katecppplugin"
          , Kate::MainWindow::Bottom
          , SmallIcon("source-cpp11")
          , i18n("C++ Helper")
          )
      }
  , m_tool_view_interior{new Ui_PluginToolViewWidget()}
  , m_includes_list_model{new QStandardItemModel()}
  , m_search_results_sortable_model{new QSortFilterProxyModel(this)}
  , m_last_explored_document{nullptr}
{
    assert("Sanity check" && m_tool_view);

    //BEGIN Setup plugin actions
    {
        auto* const actions = actionCollection();
        actions->addAction("file_open_included_header", this, SLOT(openHeader()));
        actions->addAction("file_open_switch_iface_impl", this, SLOT(switchIfaceImpl()));
        actions->addAction("cpphelper_popup_toggle_include_style", this, SLOT(toggleIncludeStyle()));
        actions->addAction("cpphelper_popup_goto_declaration", this, SLOT(gotoDeclarationUnderCursor()));
        actions->addAction("cpphelper_popup_goto_definition", this, SLOT(gotoDefinitionUnderCursor()));
        actions->addAction("cpphelper_popup_search_text", this, SLOT(searchSymbolUnderCursor()));
        actions->addAction("edit_copy_include", this, SLOT(copyInclude()));
        {
            auto* const action = actions->addAction(
                "cpphelper_popup_back_to_last_location"
              , this
              , SLOT(backToPreviousLocation())
              );
            action->setEnabled(false);                          // Initially there is no back stack
        }
    }

    // ATTENTION Add self as KXMLGUIClient after all actions has
    // been added...
    mainWindow()->guiFactory()->addClient(this);
    // ATTENTION ... so at this point searching for particular
    // submenu using factory will be successed!

    {
        auto* const my_pop = qobject_cast<QMenu*>(
            mainWindow()->guiFactory()->container("cpphelper_popup", this)
          );
        connect(my_pop, SIGNAL(aboutToShow()), this, SLOT(aboutToShow()));
    }
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
        auto* const clear_action = actionCollection()->addAction(
            "cpphelper_diagnostic_popup_clear"
          , &m_diagnostic_data
          , SLOT(clear())
          );
        m_tool_view_interior->diagnosticMessages->insertAction(nullptr, clear_action);
    }

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
      , SIGNAL(diagnosticMessage(clang::diagnostic_message))
      , this
      , SLOT(addDiagnosticMessage(clang::diagnostic_message))
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
    connect(
        m_tool_view_interior->indexFunctionBody
      , SIGNAL(toggled(bool))
      , &m_plugin->databaseManager()
      , SLOT(indexLocalsToggled(bool))
      );
    connect(
        m_tool_view_interior->skipImplicits
      , SIGNAL(toggled(bool))
      , &m_plugin->databaseManager()
      , SLOT(indexImplicitsToggled(bool))
      );
    connect(
        &m_plugin->databaseManager()
      , SIGNAL(setSkipImplicitsChecked(bool))
      , m_tool_view_interior->skipImplicits
      , SLOT(setChecked(bool))
      );
    connect(
        &m_plugin->databaseManager()
      , SIGNAL(setIndexLocalsChecked(bool))
      , m_tool_view_interior->indexFunctionBody
      , SLOT(setChecked(bool))
      );

    // Search tab
    {
        m_search_results_sortable_model->setSourceModel(&m_search_results_model);
        m_tool_view_interior->searchResults->setModel(m_search_results_sortable_model);
        connect(
            &m_search_results_model
          , SIGNAL(modelReset())
          , this
          , SLOT(searchResultsUpdated())
          );
    }
    connect(
        m_tool_view_interior->searchResults
      , SIGNAL(activated(const QModelIndex&))
      , this
      , SLOT(searchResultActivated(const QModelIndex&))
      );
    connect(
        m_tool_view_interior->searchQuery
      , SIGNAL(returnPressed())
      , this
      , SLOT(startSearchDisplayResults())
      );
}

CppHelperPluginView::~CppHelperPluginView()
{
    mainWindow()->guiFactory()->removeClient(this);
}

void CppHelperPluginView::readSessionConfig(KConfigBase* const config, const QString& groupPrefix)
{
    kDebug(DEBUG_AREA) << "** VIEW **: Reading session config: " << groupPrefix;
    KConfigGroup scg(config, groupPrefix);
    {
        auto explorer_widths = scg.readEntry(EXPLORER_TREE_WIDTH_KEY, QList<int>{});
        m_tool_view_interior->includesSplitter->setSizes(explorer_widths);
    }
    {
        auto search_widths = scg.readEntry(SEARCH_PANE_WIDTH_KEY,  QList<int>{});
        m_tool_view_interior->searchSplitter->setSizes(search_widths);
    }
    {
        auto indices_width = scg.readEntry(INDICES_LIST_WIDTH_KEY, QList<int>{});
        m_tool_view_interior->indicesSplitter->setSizes(indices_width);
    }
    {
        auto order = scg.readEntry(TOOL_VIEW_TABS_ORDER_KEY, QList<int>{});
        for (auto i = 0; i < order.size(); ++i)
            m_tool_view_interior->tabs->moveTab(order[i], i);
        m_tool_view_interior->tabs->setCurrentIndex(0);
    }
}

void CppHelperPluginView::writeSessionConfig(KConfigBase* const config, const QString& groupPrefix)
{
    kDebug(DEBUG_AREA) << "** VIEW **: Writing session config: " << groupPrefix;
    KConfigGroup scg(config, groupPrefix);

    scg.writeEntry(SEARCH_PANE_WIDTH_KEY, m_tool_view_interior->searchSplitter->sizes());
    scg.writeEntry(EXPLORER_TREE_WIDTH_KEY, m_tool_view_interior->includesSplitter->sizes());
    scg.writeEntry(INDICES_LIST_WIDTH_KEY, m_tool_view_interior->indicesSplitter->sizes());

    QList<int> order;
    auto& tw = *m_tool_view_interior;
    for (auto* tab : {tw.diagnosticTab, tw.includesTab, tw.searchTab, tw.indexerSettingsTab})
        order << m_tool_view_interior->tabs->indexOf(tab);
    kDebug(DEBUG_AREA) << "tabs order:" << order;
    scg.writeEntry(TOOL_VIEW_TABS_ORDER_KEY, order);

    scg.sync();
}

void CppHelperPluginView::addDiagnosticMessage(clang::diagnostic_message record)
{
    m_diagnostic_data.append(std::move(record));
}

/**
 * \todo What if view/document will change mime type? (after save as... for example)?
 * Maybe better to check highlighting style? For example wen new document just created,
 * but highlighted as C++ the code below wouldn't work! Even more, after document gets
 * saved to disk completion still wouldn't work! That is definitely SUXX
 */
void CppHelperPluginView::viewCreated(KTextEditor::View* const view)
{
    kDebug(DEBUG_AREA) << "view created";

    // Try to execute initial completers registration and #includes scanning
    if (handleView(view))
        m_plugin->updateDocumentInfoFromView(view);

    auto* const th_iface = qobject_cast<KTextEditor::TextHintInterface*>(view);
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

    auto* const doc = view->document();
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
      , m_plugin
      , SLOT(updateDocumentInfo(KTextEditor::Document*, const KTextEditor::Range&))
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

void CppHelperPluginView::modeChanged(KTextEditor::Document* const doc)
{
    kDebug(DEBUG_AREA) << "hl mode has been changed: " << doc->highlightingMode() << ", " << doc->mimeType();
    if (handleView(doc->activeView()))
        m_plugin->updateDocumentInfo(doc);
}

void CppHelperPluginView::urlChanged(KTextEditor::Document* const doc)
{
    kDebug(DEBUG_AREA) << "name or URL has been changed: " << doc->url() << ", " << doc->mimeType();
    if (handleView(doc->activeView()))
        m_plugin->updateDocumentInfo(doc);
}

void CppHelperPluginView::onDocumentClose(KTextEditor::Document* const doc)
{
    if (doc == m_last_explored_document)
    {
        m_last_explored_document = nullptr;
        m_tool_view_interior->includesTree->clear();
        m_includes_list_model->clear();
    }
}

void CppHelperPluginView::diagnosticMessageActivated(const QModelIndex& index)
{
    const auto loc = m_diagnostic_data.getLocationByIndex(index);
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

void CppHelperPluginView::aboutToShow()
{
    assert(
        "Active view suppose to be valid at this point! Am I wrong?"
      && mainWindow()->activeView()
      );

    const auto* const view = mainWindow()->activeView();
    auto* const doc = view->document();

    // Disable lookup actions for non C/C++ sources
    auto symbol = symbolUnderCursor();
    const auto is_suitable_document = isSuitableDocument(doc->mimeType(), doc->highlightingMode());
    const auto should_enable_lookup_action = is_suitable_document
      && !symbol.isEmpty()
      && !m_plugin->config().enabledIndices().empty()
      ;

    if (should_enable_lookup_action)
    {
        kDebug(DEBUG_AREA) << "current word text: " << symbol;
        symbol = KStringHandler::csqueeze(symbol.replace('\n', ' '), 30);
        stateChanged("has_symbol_under_cursor");
    }
    else
    {
        symbol = "...";
        stateChanged("has_symbol_under_cursor", KXMLGUIClient::StateReverse);
    }
    kDebug(DEBUG_AREA) << "symbol =" << symbol;

    {
        auto* const actions = actionCollection();
        actions->action("cpphelper_popup_goto_declaration")->setText(
            i18nc("@action:inmenu", "Go to Declaration of <icode>%1</icode>", symbol)
          );
        actions->action("cpphelper_popup_goto_definition")->setText(
            i18nc("@action:inmenu", "Go to Definition of <icode>%1</icode>", symbol)
          );
        actions->action("cpphelper_popup_search_text")->setText(
            i18nc("@action:inmenu", "Search for <icode>%1</icode>", symbol)
          );
    }

    // Disable #include style switcher if no include on a line
    if (!view->selection())
    {
        const auto line_str = doc->line(view->cursorPosition().line());
        const auto r = parseIncludeDirective(line_str, true);
        stateChanged(
            "no_include_at_current_line"
          , r.range.isValid()
            ? KXMLGUIClient::StateReverse
            : KXMLGUIClient::StateNoReverse
          );
    }
}

/**
 * Check if view has \c KTextEditor::CodeCompletionInterface and do nothing if it doesn't.
 * Otherwise, check if current document has suitable type (C/C++ source/header):
 * \li if yes, check if completers already registered, and do register if they don't
 * \li if not suitable document, check if any completers was registered for a given view
 * and unregister if it was.
 *
 * \param view view to register completers for. Do nothing if \c nullptr.
 * \return \c true if suitable document and registration successed, \c false otherwise.
 */
bool CppHelperPluginView::handleView(KTextEditor::View* const view)
{
    if (!view)                                              // Null view is quite possible...
        return false;                                       // do nothing in this case.

    const auto is_suitable_document =
        isSuitableDocument(view->document()->mimeType(), view->document()->highlightingMode());

    updateCppActionsAvailability(is_suitable_document);

    auto* const cc_iface = qobject_cast<KTextEditor::CodeCompletionInterface*>(view);
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
            auto include_completer = std::make_unique<IncludeHelperCompletionModel>(
                view
              , m_plugin
              );
            auto code_completer = std::make_unique<ClangCodeCompletionModel>(
                view
              , m_plugin
              , m_diagnostic_data
              );
            auto preprocessor_completer = std::make_unique<PreprocessorCompletionModel>(
                view
              , m_plugin
              , m_diagnostic_data
              );
            auto r = m_completers.insert(
                std::make_pair(
                    view
                  , std::make_tuple(
                        include_completer.release()
                      , code_completer.release()
                      , preprocessor_completer.release()
                      )
                  )
              );
            assert("Completers expected to be new" && r.second);
            // Enable #include completions
            cc_iface->registerCompletionModel(std::get<0>(r.first->second));
            // Enable semantic C++ code completions
            cc_iface->registerCompletionModel(std::get<1>(r.first->second));
            // Enable preprocessor directives completion
            cc_iface->registerCompletionModel(std::get<2>(r.first->second));
            // Turn auto completions ON
            cc_iface->setAutomaticInvocationEnabled(true);
            result = true;
        }
    }
    else
    {
        // Oops! No completers required for this document.
        // Check if some was registered before, and erase if it is
        if (it != end(m_completers))
        {
            kDebug(DEBUG_AREA) << "Not a C/C++ source (anymore): unregister code completers";
            cc_iface->unregisterCompletionModel(std::get<0>(it->second));
            cc_iface->unregisterCompletionModel(std::get<1>(it->second));
            cc_iface->unregisterCompletionModel(std::get<2>(it->second));
            /// \todo Is there any damn way to avoid explicit delete???
            /// Fraking Qt (as far as I know) was developed w/ automatic
            /// memory management (to help users not to think about it),
            /// so WHY I still have to call \c new and/or \c delete !!!
            /// FRAKING WHY!?
            delete std::get<0>(it->second);
            delete std::get<1>(it->second);
            delete std::get<2>(it->second);
            m_completers.erase(it);
        }
    }
    kDebug(DEBUG_AREA) << "RESULT:" << result;
    return result;
}

bool CppHelperPluginView::eventFilter(QObject* const obj, QEvent* const event)
{
    if (event->type() == QEvent::KeyPress)
    {
        const auto* const ke = static_cast<QKeyEvent*>(event);
        if ((obj == m_tool_view.get()) && (ke->key() == Qt::Key_Escape))
        {
            mainWindow()->hideToolView(m_tool_view.get());
            event->accept();
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

/**
 * Enable/disable C++ specific actions (copy \c #include, update button in explorer, etc.)
 */
void CppHelperPluginView::updateCppActionsAvailability()
{
    const auto* const view = mainWindow()->activeView();
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

    m_tool_view_interior->updateButton->setEnabled(enable_cpp_specific_actions);

    actionCollection()->action("edit_copy_include")->setText(
        enable_cpp_specific_actions
      ? i18nc("@action:inmenu",  "Copy #include to Clipboard")
      : i18nc("@action:inmenu",  "Copy File URI to Clipboard")
      );
    stateChanged(
        "cpp_actions_enabled"
      , enable_cpp_specific_actions
          ? KXMLGUIClient::StateNoReverse
          : KXMLGUIClient::StateReverse
      );
}

//END CppHelperPluginView
}                                                           // namespace kate
// kate: hl C++/Qt4;
