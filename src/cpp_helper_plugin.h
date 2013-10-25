/**
 * \file
 *
 * \brief Class \c kate::CppHelperPlugin (interface)
 *
 * \date Sun Jan 29 09:15:53 MSK 2012 -- Initial design
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
#include <src/clang/disposable.h>
#include <src/clang/unsaved_files_list.h>
#include <src/database_manager.h>
#include <src/header_files_cache.h>
#include <src/plugin_configuration.h>
#include <src/translation_unit.h>

// Standard includes
#include <kate/plugin.h>
#include <kate/pluginconfigpageinterface.h>
#include <KDE/KTextEditor/Document>
#include <KDE/KTextEditor/HighlightInterface>
#include <KDE/KDirWatch>

#include <cassert>
#include <map>
#include <memory>

namespace kate {
class DocumentInfo;                                         // forward declaration

/**
 * \brief The plugin class
 *
 * This class do the following tasks:
 *
 * \li Implementation of required (inherited) functions
 * \li Delegate configuration read/write to \c m_config
 * \li Create a hidden document and use it to highlight C++ code snippets from code completer
 * \li Maintain a mapping of documents (as pointers) to document info (a list of ranges
 *     with \c #include directives)
 */
class CppHelperPlugin
  : public Kate::Plugin
  , public Kate::PluginConfigPageInterface
{
    Q_OBJECT
    Q_INTERFACES(Kate::PluginConfigPageInterface)

public:
    explicit CppHelperPlugin(QObject* = 0, const QList<QVariant>& = QList<QVariant>());
    virtual ~CppHelperPlugin();

    /// \name Accessors
    //@{
    PluginConfiguration& config();
    const PluginConfiguration& config() const;
    CXIndex localIndex() const;
    CXIndex index() const;
    HeaderFilesCache& headersCache();
    const HeaderFilesCache& headersCache() const;
    DatabaseManager& databaseManager();
    const DatabaseManager& databaseManager() const;
    const clang::unsaved_files_list& unsavedFiles() const;
    //@}

    /// \name \c Kate::PluginConfigPageInterface interface implementation
    //@{
    /// Get number of configuration pages for this plugin
    virtual uint configPages() const override;
    /// Create a config page w/ given number and parent
    virtual Kate::PluginConfigPage* configPage(uint = 0, QWidget* = 0, const char* = 0) override;
    /// Get short name of a config page by number
    virtual QString configPageName(uint = 0) const override;
    virtual QString configPageFullName(uint = 0) const override;
    virtual KIcon configPageIcon(uint = 0) const override;
    //@}

    /// \name \c Kate::Plugin interface implementation
    //@{
    virtual void readSessionConfig(KConfigBase*, const QString&) override;
    virtual void writeSessionConfig(KConfigBase*, const QString&) override;
    /// Create a new view of this plugin for the given main window
    virtual Kate::PluginView* createView(Kate::MainWindow*) override;
    //@}

    /// Highlight given snippet using internal (hidden) \c KTextEditor::Document
    QList<KTextEditor::HighlightInterface::AttributeBlock> highlightSnippet(
        const QString&
      , const QString&
      );
    /// Helper function to collect unsaved files from current editor
    void updateUnsavedFiles();
    TranslationUnit& getTranslationUnitByDocument(KTextEditor::Document*, bool = true);
    DocumentInfo& getDocumentInfo(KTextEditor::Document*);
    void addDiagnosticMessage(DiagnosticMessagesModel::Record);

Q_SIGNALS:
    void diagnosticMessage(DiagnosticMessagesModel::Record);

public Q_SLOTS:
    void updateDocumentInfo(KTextEditor::Document*);
    void removeDocumentInfo(KTextEditor::Document*);
    void openDocument(const KUrl&);
    void makePCHFile(const KUrl&);

private Q_SLOTS:
    void createdPath(const QString&);
    void deletedPath(const QString&);
    void updateCurrentView();
    void buildPCHIfAbsent();                                ///< Make sure a PCH is fresh
    /// Update watcher to monitor currently configured directories
    void updateDirWatcher();
    void invalidateTranslationUnits();

private:
    /// Type to associate a document with a translation unit
    typedef std::map<
        KTextEditor::Document*
      , std::pair<
            std::unique_ptr<TranslationUnit>                // local translation unit (w/ PCH enabled)
          , std::unique_ptr<TranslationUnit>                // raw translation unit (w/o PCH)
          >
      > translation_units_map_type;
    /// Type to associate a document with a collection of \c #include file ranges
    /// (i.e. \c DocumentInfo)
    typedef std::map<
        KTextEditor::Document*
      , std::unique_ptr<DocumentInfo>
      > doc_info_type;

    /// Update watcher to monitor a given entry
    void updateDirWatcher(const QString&);
    TranslationUnit& getTranslationUnitByDocumentImpl(
        KTextEditor::Document*
      , clang::DCXIndex&
      , std::unique_ptr<TranslationUnit> translation_units_map_type::mapped_type::*
      , const unsigned
      , const bool
      );
    /// Obtain a pointer to an internally used (hidden) document
    KTextEditor::Document* getHiddenDoc();

    /// An instance of \c PluginConfiguration filled with configuration data
    /// read from application's config
    PluginConfiguration m_config;
    /// Clang-C index instance used by code completer
    clang::DCXIndex m_local_index;
    /// Clang-C index instance used by \c #include explorer
    clang::DCXIndex m_index;
    /// A map of \c KTextEditor::Document pointer to \c DocumentInfo
    doc_info_type m_doc_info;
    /// Directory watcher to monitor configured directories
    std::unique_ptr<KDirWatch> m_dir_watcher;
    /// Index databases manager
    DatabaseManager m_db_mgr;
    /// \note Directory watcher reports about 4 times just for one event,
    /// so to avoid doing stupid job, lets remember what we've done the last time.
    QString m_last_updated;
    /// A never shown document used to highlight code completion items
    KTextEditor::Document* m_hidden_doc;
    translation_units_map_type m_units;
    HeaderFilesCache m_headers_cache;
    clang::unsaved_files_list m_unsaved_files_cache;
};

inline PluginConfiguration& CppHelperPlugin::config()
{
    return m_config;
}
inline const PluginConfiguration& CppHelperPlugin::config() const
{
    return m_config;
}
inline CXIndex CppHelperPlugin::localIndex() const
{
    return m_local_index;
}
inline CXIndex CppHelperPlugin::index() const
{
    return m_index;
}
inline HeaderFilesCache& CppHelperPlugin::headersCache()
{
    return m_headers_cache;
}
inline const HeaderFilesCache& CppHelperPlugin::headersCache() const
{
    return m_headers_cache;
}
inline DatabaseManager& CppHelperPlugin::databaseManager()
{
    return m_db_mgr;
}
inline const DatabaseManager& CppHelperPlugin::databaseManager() const
{
    return m_db_mgr;
}

inline const clang::unsaved_files_list& CppHelperPlugin::unsavedFiles() const
{
    return m_unsaved_files_cache;
}

inline TranslationUnit& CppHelperPlugin::getTranslationUnitByDocument(
    KTextEditor::Document* doc
  , const bool is_local_requested
  )
{
    auto parse_options = is_local_requested
      ? TranslationUnit::defaultEditingParseOptions()
      : TranslationUnit::defaultExplorerParseOptions()
      ;
    return getTranslationUnitByDocumentImpl(
        doc
      , is_local_requested ? m_local_index : m_index
      , is_local_requested
        ? &translation_units_map_type::mapped_type::first
        : &translation_units_map_type::mapped_type::second
      , parse_options
      , is_local_requested
      );
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
