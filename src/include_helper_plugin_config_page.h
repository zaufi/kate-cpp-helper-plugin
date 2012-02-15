/**
 * \file
 *
 * \brief Class \c kate::include_helper_plugin_config_page (interface)
 *
 * \date Mon Feb  6 06:04:17 MSK 2012 -- Initial design
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

#ifndef __SRC__INCLUDE_HELPER_PLUGIN_CONFIG_PAGE_H__
#  define __SRC__INCLUDE_HELPER_PLUGIN_CONFIG_PAGE_H__

// Project specific includes
#  include <src/ui_path_config.h>
#  include <src/ui_other_settings.h>

// Standard includes
#  include <kate/plugin.h>
#  include <kate/pluginconfigpageinterface.h>

namespace kate {
class IncludeHelperPlugin;                                  // forward declaration

/**
 * \brief Configuration page for the plugin
 *
 * [More detailed description here]
 *
 */
class IncludeHelperPluginConfigPage : public Kate::PluginConfigPage
{
    Q_OBJECT

public:
    explicit IncludeHelperPluginConfigPage(QWidget* = 0, IncludeHelperPlugin* = 0);
    virtual ~IncludeHelperPluginConfigPage() {}

    /// \name PluginConfigPage interface implementation
    //@{
    void apply();
    void reset();
    void defaults() {}
    //@}

Q_SIGNALS:
    void sessionDirsUpdated(const QStringList&);
    void globalDirsUpdated(const QStringList&);

private Q_SLOTS:
    void addGlobalIncludeDir();                             ///< Add directory to the list
    void delGlobalIncludeDir();                             ///< Remove directory from the list
    void moveGlobalDirUp();
    void moveGlobalDirDown();
    void addSessionIncludeDir();                            ///< Add directory to the list
    void delSessionIncludeDir();                            ///< Remove directory from the list
    void moveSessionDirUp();
    void moveSessionDirDown();

private:
    bool contains(const QString&, const KListWidget*);      ///< Check if directories list contains given item

    IncludeHelperPlugin* m_plugin;                          ///< Parent plugin
    Ui_PerSessionSettingsConfigWidget* const m_pss_config;
    Ui_PathListConfigWidget* const m_system_list;
    Ui_PathListConfigWidget* const m_session_list;
};

}                                                           // namespace kate
#endif                                                      // __SRC__INCLUDE_HELPER_PLUGIN_CONFIG_PAGE_H__
