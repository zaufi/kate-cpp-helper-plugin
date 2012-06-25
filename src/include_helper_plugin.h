/**
 * \file
 *
 * \brief Class \c kate::IncludeHelperPlugin (interface)
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

#ifndef __SRC__INCLUDE_HELPER_PLUGIN_HH__
#  define __SRC__INCLUDE_HELPER_PLUGIN_HH__

// Project specific includes

// Standard includes
#  include <kate/plugin.h>
#  include <kate/pluginconfigpageinterface.h>
#  include <KTextEditor/Document>
#  include <cassert>

namespace kate {
class DocumentInfo;                                         // forward declaration

/**
 * \brief [Type brief class description here]
 *
 * [More detailed description here]
 *
 */
class IncludeHelperPlugin : public Kate::Plugin, public Kate::PluginConfigPageInterface
{
    Q_OBJECT
    Q_INTERFACES(Kate::PluginConfigPageInterface)

public:
    typedef QMap<KTextEditor::Document*, DocumentInfo*> doc_info_type;

    explicit IncludeHelperPlugin(QObject* = 0, const QList<QVariant>& = QList<QVariant>());
    virtual ~IncludeHelperPlugin();

    /// Create a new view of this plugin for the given main window
    Kate::PluginView* createView(Kate::MainWindow*);

    /// \name Accessors
    //@{
    const QStringList& sessionDirs() const
    {
        return m_session_dirs;
    }
    const QStringList& systemDirs() const
    {
        return m_system_dirs;
    }
    bool useLtGt() const
    {
        return m_use_ltgt;
    }
    bool useCwd() const
    {
        return m_use_cwd;
    }
    const doc_info_type& managed_docs() const
    {
        return m_doc_info;
    }
    doc_info_type& managed_docs()
    {
        return m_doc_info;
    }
    //@}

    /// \name Modifiers
    //@{
    void setSessionDirs(QStringList& dirs);
    void setGlobalDirs(QStringList& dirs);
    void setUseLtGt(const bool state);
    void setUseCwd(const bool state);
    //@}

    /// \name PluginConfigPageInterface interface implementation
    //@{
    /// Get number of configuration pages for this plugin
    uint configPages() const
    {
        return 1;
    }
    /// Create a config page w/ given number and parent
    Kate::PluginConfigPage* configPage(uint = 0, QWidget* = 0, const char* = 0);
    /// Get short name of a config page by number
    QString configPageName(uint number = 0) const
    {
        Q_UNUSED(number)
        assert("This plugin have the only configuration page" && number == 0);
        return "Include Helper";
    }
    QString configPageFullName(uint number = 0) const
    {
        Q_UNUSED(number)
        assert("This plugin have the only configuration page" && number == 0);
        return "Inlcude Helper Settings";
    }
    KIcon configPageIcon(uint number = 0) const
    {
        Q_UNUSED(number)
        assert("This plugin have the only configuration page" && number == 0);
        return KIcon("text-x-c++hdr");
    }
    //@}

    /// \name Plugin interface implementation
    //@{
    void readSessionConfig(KConfigBase*, const QString&);
    void writeSessionConfig(KConfigBase*, const QString&);
    //@}
    void readConfig();                                      ///< Read global config

Q_SIGNALS:
    void sessionDirsChanged();
    void systemDirsChanged();

private:
    QStringList m_system_dirs;
    QStringList m_session_dirs;
    doc_info_type m_doc_info;
    /// If \c true <em>Copy #include</em> action would put filename into \c '<' and \c '>'
    /// instead of \c '"'
    bool m_use_ltgt;
    bool m_use_cwd;
    bool m_config_dirty;
};
}                                                           // namespace kate
#endif                                                      // __SRC__INCLUDE_HELPER_PLUGIN_HH__
