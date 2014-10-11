/**
 * \file
 *
 * \brief Class \c kate::CppHelperPluginView (interface)
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

#pragma once

// Project specific includes
#include "document_info.h"
#include "diagnostic_messages_model.h"
#include "search_results_table_model.h"
#include "ui_plugin_tool_view.h"

// Standard includes
#include <kate/plugin.h>
#include <KDE/KTextEditor/View>
#include <clang-c/Index.h>
#include <map>
#include <memory>
#include <stack>
#include <tuple>

class QSortFilterProxyModel;
class QStandardItemModel;
class KAction;

namespace kate { namespace details {
struct InclusionVisitorData;
}                                                           // namespace details
class CppHelperPlugin;
class IncludeHelperCompletionModel;
class ClangCodeCompletionModel;
class PreprocessorCompletionModel;


/**
 * \brief [Type brief class description here]
 *
 * \note When some view going to be deleted, it will try to delete all
 * child \c QObject (and completers are children of the view, btw)
 * so when we want to deregister completers we have to use \b explicit \c delete
 * and there is no way to use RAII heres (like \c std::unique_ptr) cuz when
 * document gets close there is no views already and completers actually deleted
 * at this moment... so if we want to avoid explicit delete (at one place)
 * we'll have to use explicit \c std::unique_ptr::release twice!
 * FRAK IT!
 *
 * \todo Add some suffix to slots? ORLY?
 *
 * \todo Use \c KUrl for file references
 *
 */
class CppHelperPluginView
  : public Kate::PluginView
  , public Kate::XMLGUIClient
{
    Q_OBJECT

public:
    CppHelperPluginView(Kate::MainWindow*, const KComponentData&, CppHelperPlugin*);
    virtual ~CppHelperPluginView();

    /// \name PluginView interface implementation
    //@{
    virtual void readSessionConfig(KConfigBase*, const QString&) override;
    virtual void writeSessionConfig(KConfigBase*, const QString&) override;
    //@}

public Q_SLOTS:
    void addDiagnosticMessage(clang::diagnostic_message);

private Q_SLOTS:
    /// \name Kate event handlers
    //@{
    void viewCreated(KTextEditor::View*);
    void modeChanged(KTextEditor::Document*);
    void urlChanged(KTextEditor::Document*);
    void onDocumentClose(KTextEditor::Document*);
    void aboutToShow();
    void updateCppActionsAvailability();                    ///< Enable/disable C++ specific actions in UI
    void diagnosticMessageActivated(const QModelIndex&);
    //@}

    /// \name Document services
    //@{
    /// Open a single document
    void switchIfaceImpl();                                 ///< Open corresponding header/implementation file
    void openFile(const KUrl&, KTextEditor::Cursor = KTextEditor::Cursor::invalid(), bool = true);
    void openHeader();                                      ///< Open header file under cursor
    void toggleIncludeStyle();
    /// From \c #include directive w/ current file in the clipboard
    void copyInclude();
    void needTextHint(const KTextEditor::Cursor&, QString&);
    //@}

    /// \name #include Explorer related slots
    //@{
    void updateInclusionExplorer();
    void includeFileActivatedFromTree(QTreeWidgetItem*, int);
    void includeFileDblClickedFromTree(QTreeWidgetItem*, int);
    void includeFileDblClickedFromList(const QModelIndex&);
    //@}

    /// \name Search services
    //@{
    void gotoDeclarationUnderCursor();
    void gotoDefinitionUnderCursor();
    void searchSymbolUnderCursor();
    void backToPreviousLocation();
    void playgroundAction();
    void reindexingStarted(const QString&);
    void reindexingFinished(const QString&);
    void startSearchDisplayResults();
    void searchResultsUpdated();
    void searchResultActivated(const QModelIndex&);
    void locationLinkActivated(const QString&);
    //@}

private:
    /// Type to hold a completers associated with a view
    typedef std::map<
        KTextEditor::View*
      , std::tuple<
            IncludeHelperCompletionModel*
          , ClangCodeCompletionModel*
          , PreprocessorCompletionModel*
          >
      > completions_models_map_type;

    /// Enable/disable C++ specific actions in UI
    void updateCppActionsAvailability(const bool);
    /// Register or deregister completion models for a given view
    bool handleView(KTextEditor::View*);
    bool eventFilter(QObject*, QEvent*);

    /// Try to find file(s) w/ a given name+path and a list of possible extensions
    QStringList findCandidatesAt(const QString&, const QString&, const QStringList&);
    /// Try to get an \c #include filename under cursor as range
    IncludeParseResult findIncludeFilenameNearCursor() const;
    /// Get list of absolute paths to filename
    QStringList findFileLocations(const QString&, bool, bool = true);

    void inclusionVisitor(details::InclusionVisitorData*, CXFile, CXSourceLocation*, unsigned);
    void dblClickOpenFile(QString&&);

    void setSearchQueryAndShowIt(const QString&);
    void appendSearchDetailsRow(const QString&, const QString&, bool = true);
    void appendSearchDetailsRow(const QString&, bool);
    void clearSearchDetails();
    QString symbolUnderCursor();
    void toggleIncludeStyle(KTextEditor::Document*, int, int);
    QString tryGuessHeaderRelativeConfiguredDirs(QString, QFileInfo);

    CppHelperPlugin* const m_plugin;                        ///< Parent plugin
    std::unique_ptr<QWidget> m_tool_view;                   ///< A tool-view widget of this plugin
    Ui_PluginToolViewWidget* const m_tool_view_interior;    ///< Interior widget of a tool-view
    QStandardItemModel* const m_includes_list_model;
    QSortFilterProxyModel* const m_search_results_sortable_model;
    KTextEditor::Document* m_last_explored_document;        ///< Document explored in the \c #includes view

    DiagnosticMessagesModel m_diagnostic_data;              ///< Storage (model) for diagnostic messages
    completions_models_map_type m_completers;               ///< Registered completers by view
    SearchResultsTableModel m_search_results_model;         ///< Model to hold search results
    std::stack<clang::location> m_recent_locations;         ///< Stack of last locations
};

}                                                           // namespace kate
// kate: hl C++/Qt4;
