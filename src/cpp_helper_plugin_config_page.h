/**
 * \file
 *
 * \brief Class \c kate::CppHelperPluginConfigPage (interface)
 *
 * \date Mon Feb  6 06:04:17 MSK 2012 -- Initial design
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

#ifndef __SRC__INCLUDE_HELPER_PLUGIN_CONFIG_PAGE_H__
# define __SRC__INCLUDE_HELPER_PLUGIN_CONFIG_PAGE_H__

// Project specific includes
# include <src/ui_clang_settings.h>
# include <src/ui_detect_compiler_paths.h>
# include <src/ui_other_settings.h>
# include <src/ui_path_config.h>
# include <src/ui_session_paths_sets.h>

// Standard includes
# include <kate/plugin.h>
# include <kate/pluginconfigpageinterface.h>
# include <KProcess>
# include <KSharedConfig>
# include <map>

namespace kate {
class CppHelperPlugin;                                  // forward declaration

/**
 * \brief Configuration page for the plugin
 *
 * [More detailed description here]
 *
 */
class CppHelperPluginConfigPage : public Kate::PluginConfigPage
{
    Q_OBJECT

public:
    explicit CppHelperPluginConfigPage(QWidget* = 0, CppHelperPlugin* = 0);
    virtual ~CppHelperPluginConfigPage() {}

    /// \name \c Kate::PluginConfigPage interface implementation
    //@{
    void apply();
    void reset();
    void defaults() {}
    //@}

Q_SIGNALS:
    void sessionDirsUpdated(const QStringList&);
    void systemDirsUpdated(const QStringList&);

private Q_SLOTS:
    void addGlobalIncludeDir();                             ///< Add directory to the list
    void delGlobalIncludeDir();                             ///< Remove directory from the list
    void moveGlobalDirUp();
    void moveGlobalDirDown();
    void clearGlobalDirs();
    void addSessionIncludeDir();                            ///< Add directory to the list
    void delSessionIncludeDir();                            ///< Remove directory from the list
    void moveSessionDirUp();
    void moveSessionDirDown();
    void clearSessionDirs();
    void addSet();
    void guessSet();
    void storeSet();
    void openPCHHeaderFile();                               ///< Open configured PCH header
    void rebuildPCH();                                      ///< Rebuild a PCH file from configured header
    void pchHeaderChanged(const QString&);
    void pchHeaderChanged(const KUrl&);
    void detectPredefinedCompilerPaths();                   ///< Detect predefined compiler paths pressed
    void error(QProcess::ProcessError);                     ///< Compiler process failed to start
    void finished(int, QProcess::ExitStatus);               ///< Compiler process finished
    void readyReadStandardOutput();                         ///< Ready to read standard input
    void readyReadStandardError();                          ///< Ready to read standard error

private:
    bool contains(const QString&, const KListWidget*);      ///< Check if directories list contains given item
    QString findBinary(const QString&) const;
    QString getCurrentCompiler() const;
    void addDirTo(const KUrl&, KListWidget*);
    void updateSets();                                      ///< Update predefined \c #include sets

    CppHelperPlugin* m_plugin;                              ///< Parent plugin
    Ui_PerSessionSettingsConfigWidget* const m_pss_config;
    Ui_CLangOptionsWidget* const m_clang_config;
    Ui_PathListConfigWidget* const m_system_list;
    Ui_PathListConfigWidget* const m_session_list;
    Ui_DetectCompilerPathsWidget* const m_compiler_paths;
    Ui_SessionPathsSetsWidget* const m_favorite_sets;
    KProcess m_compiler_proc;
    QString m_output;
    QString m_error;
    std::map<QString, KSharedConfigPtr> m_include_sets;
};

}                                                           // namespace kate
#endif                                                      // __SRC__INCLUDE_HELPER_PLUGIN_CONFIG_PAGE_H__
// kate: hl C++11/Qt4;
