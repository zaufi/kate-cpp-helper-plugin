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

#ifndef __SRC__INCLUDE_HELPER_PLUGIN_HH__
# define __SRC__INCLUDE_HELPER_PLUGIN_HH__

// Project specific includes
# include <src/clang_utils.h>
# include <src/plugin_configuration.h>

// Standard includes
# include <kate/plugin.h>
# include <kate/pluginconfigpageinterface.h>
# include <KTextEditor/Document>
# include <KDirWatch>
# include <cassert>
# include <map>
# include <memory>

namespace kate {
class DocumentInfo;                                         // forward declaration

/**
 * \brief [Type brief class description here]
 *
 * [More detailed description here]
 *
 */
class CppHelperPlugin
  : public Kate::Plugin
  , public Kate::PluginConfigPageInterface
{
    Q_OBJECT
    Q_INTERFACES(Kate::PluginConfigPageInterface)

public:
    typedef std::map<KTextEditor::Document*, std::unique_ptr<DocumentInfo>> doc_info_type;

    explicit CppHelperPlugin(QObject* = 0, const QList<QVariant>& = QList<QVariant>());
    virtual ~CppHelperPlugin();

    /// \name Accessors
    //@{
    PluginConfiguration& config();
    const PluginConfiguration& config() const;
    const doc_info_type& managed_docs() const;
    doc_info_type& managed_docs();
    CXIndex index() const;
    //@}

    /// \name \c Kate::PluginConfigPageInterface interface implementation
    //@{
    /// Get number of configuration pages for this plugin
    uint configPages() const;
    /// Create a config page w/ given number and parent
    Kate::PluginConfigPage* configPage(uint = 0, QWidget* = 0, const char* = 0);
    /// Get short name of a config page by number
    QString configPageName(uint = 0) const;
    QString configPageFullName(uint = 0) const;
    KIcon configPageIcon(uint = 0) const;
    //@}

    /// \name \c Kate::Plugin interface implementation
    //@{
    void readSessionConfig(KConfigBase*, const QString&);
    void writeSessionConfig(KConfigBase*, const QString&);
    /// Create a new view of this plugin for the given main window
    Kate::PluginView* createView(Kate::MainWindow*);
    //@}

public Q_SLOTS:
    void updateDocumentInfo(KTextEditor::Document* doc);
    void textInserted(KTextEditor::Document*, const KTextEditor::Range&);
    void openDocument(const KUrl&);
    void makePCHFile(const KUrl&);

private Q_SLOTS:
    void createdPath(const QString&);
    void deletedPath(const QString&);
    void updateCurrentView();
    void buildPCHIfAbsent();                                ///< Make sure a PCH is fresh
    /// Update warcher to monitor currently configured directories
    void updateDirWatcher();

private:
    /// Update warcher to monitor a given entry
    void updateDirWatcher(const QString&);

    PluginConfiguration m_config;
    DCXIndex m_index;
    doc_info_type m_doc_info;
    std::unique_ptr<KDirWatch> m_dir_watcher;
    /// \note Directory watcher reports about 4 times just for one event,
    /// so to avoid doing stupid job, lets remember what we've done the last time.
    QString m_last_updated;
};

inline PluginConfiguration& CppHelperPlugin::config()
{
    return m_config;
}
inline const PluginConfiguration& CppHelperPlugin::config() const
{
    return m_config;
}
inline auto CppHelperPlugin::managed_docs() const -> const doc_info_type&
{
    return m_doc_info;
}
inline auto CppHelperPlugin::managed_docs() -> doc_info_type&
{
    return m_doc_info;
}
inline CXIndex CppHelperPlugin::index() const
{
    return m_index;
}

}                                                           // namespace kate
#endif                                                      // __SRC__INCLUDE_HELPER_PLUGIN_HH__
// kate: hl C++11/Qt4;
