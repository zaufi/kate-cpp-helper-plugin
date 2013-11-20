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
#include <src/ui_plugin_tool_view.h>
#include <src/diagnostic_messages_model.h>
#include <src/document_info.h>

// Standard includes
#include <kate/plugin.h>
#include <KDE/KTextEditor/View>
#include <clang-c/Index.h>
#include <map>
#include <memory>

class QSortFilterProxyModel;
class QStandardItemModel;
class KAction;

namespace kate { namespace details {
struct InclusionVisitorData;
}                                                           // namespace details
class CppHelperPlugin;
class IncludeHelperCompletionModel;
class ClangCodeCompletionModel;


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
    /// Open a single document
    void openFile(const KUrl&, KTextEditor::Cursor = KTextEditor::Cursor::invalid());
    void openHeader();                                      ///< Open header file under cursor
    void switchIfaceImpl();                                 ///< Open corresponding header/implementation file
    void copyInclude();                                     ///< From \c #include directive w/ current file in the clipboard
    void viewCreated(KTextEditor::View*);
    void modeChanged(KTextEditor::Document*);
    void urlChanged(KTextEditor::Document*);
    void textInserted(KTextEditor::Document*, const KTextEditor::Range&);
    void searchSymbolUnderCursor();
    void needTextHint(const KTextEditor::Cursor&, QString&);
    void updateInclusionExplorer();
    void includeFileActivatedFromTree(QTreeWidgetItem*, int);
    void includeFileDblClickedFromTree(QTreeWidgetItem*, int);
    void includeFileDblClickedFromList(const QModelIndex&);
    void diagnosticMessageActivated(const QModelIndex&);
    void onDocumentClose(KTextEditor::Document*);
    void updateCppActionsAvailability();                    ///< Enable/disable C++ specific actions in UI
    void reindexingStarted(const QString&);
    void reindexingFinished(const QString&);
    void startSearch();
    void searchResultsUpdated();
    void searchResultActivated(const QModelIndex&);
    void locationLinkActivated(const QString&);
    void aboutToShow();

private:
    /// Type to hold a completers associated with a view
    typedef std::map<
        KTextEditor::View*
      , std::pair<IncludeHelperCompletionModel*, ClangCodeCompletionModel*>
      > completions_models_map_type;

    /// Register or deregister completion models for a given view
    bool handleView(KTextEditor::View*);

    /// Try to find file(s) w/ a given name+path and a list of possible extensions
    QStringList findCandidatesAt(const QString&, const QString&, const QStringList&);
    bool eventFilter(QObject*, QEvent*);

    /// Try to get an \c #include filename under cursor as range
    KTextEditor::Range findIncludeFilenameNearCursor() const;
    void openFiles(const QStringList&);                     ///< Open documents for all URIs in a given list
    QStringList findFileLocations(const QString&);          ///< Get list of absolute paths to filename
    void inclusionVisitor(details::InclusionVisitorData*, CXFile, CXSourceLocation*, unsigned);
    void dblClickOpenFile(QString&&);
    void updateCppActionsAvailability(const bool);          ///< Enable/disable C++ specific actions in UI
    void appendSearchDetailsRow(const QString&, const QString&, bool = true);
    void appendSearchDetailsRow(const QString&, bool);
    void clearSearchDetails();
    void applyToolViewInteriorWidths();
    QString symbolUnderCursor();

    CppHelperPlugin* const m_plugin;                        ///< Parent plugin
    KAction* const m_copy_include;                          ///< <em>Copy #include to clipboard</em> action
    /// Action to search definition of the symbol under cursor
    KAction* const m_search_definition;
    std::unique_ptr<QWidget> m_tool_view;                   ///< A tool-view widget of this plugin
    Ui_PluginToolViewWidget* const m_tool_view_interior;    ///< Interior widget of a tool-view
    QStandardItemModel* const m_includes_list_model;
    QSortFilterProxyModel* const m_search_results_model;
    KTextEditor::Document* m_last_explored_document;        ///< Document explored in the \c #includes view

    DiagnosticMessagesModel m_diagnostic_data;              ///< Storage (model) for diagnostic messages
    completions_models_map_type m_completers;               ///< Registered completers by view

    QList<int> m_explorer_widths;
    QList<int> m_search_widths;
    QList<int> m_indices_width;
    bool m_widths_applied = {false};
};

}                                                           // namespace kate
// kate: hl C++11/Qt4;
