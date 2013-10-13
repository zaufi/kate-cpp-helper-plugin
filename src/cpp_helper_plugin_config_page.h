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

#pragma once

// Project specific includes

// Standard includes
#include <kate/plugin.h>
#include <kate/pluginconfigpageinterface.h>
#include <KProcess>
#include <KListWidget>
#include <KSharedConfig>
#include <map>

namespace Ui {
class CLangOptionsWidget;
class CompletionSettings;
class DetectCompilerPathsWidget;
class PathListConfigWidget;
class PerSessionSettingsConfigWidget;
class SessionPathsSetsWidget;
}                                                           // namespace Ui

namespace kate {
class CppHelperPlugin;                                      // forward declaration

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

public Q_SLOTS:
    /// \name \c Kate::PluginConfigPage interface implementation
    //@{
    void apply();
    void reset();
    void defaults();
    //@}

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
    void removeSet();
    void storeSet();
    void addSuggestedDir();
    void openPCHHeaderFile();                               ///< Open configured PCH header
    void rebuildPCH();                                      ///< Rebuild a PCH file from configured header
    void pchHeaderChanged(const QString&);
    void pchHeaderChanged(const KUrl&);
    void detectPredefinedCompilerPaths();                   ///< Detect predefined compiler paths pressed
    void error(QProcess::ProcessError);                     ///< Compiler process failed to start
    void finished(int, QProcess::ExitStatus);               ///< Compiler process finished
    void readyReadStandardOutput();                         ///< Ready to read standard input
    void readyReadStandardError();                          ///< Ready to read standard error
    void updateSuggestions();
    void addEmptySanitizeRule();
    void removeSanitizeRule();
    void moveSanitizeRuleUp();
    void moveSanitizeRuleDown();
    void validateSanitizeRule(int, int);
    std::pair<bool, QString> isSanitizeRuleValid(int, int) const;

private:
    bool contains(const QString&, const KListWidget*);      ///< Check if directories list contains given item
    QString findBinary(const QString&) const;
    QString getCurrentCompiler() const;
    void addDirTo(const KUrl&, KListWidget*);
    void updateSets();                                      ///< Update predefined \c #include sets
    void swapRuleRows(int, int);                            ///< Swap rule parts in a given rows

    CppHelperPlugin* m_plugin;                              ///< Parent plugin
    Ui::PerSessionSettingsConfigWidget* const m_pss_config;
    Ui::CLangOptionsWidget* const m_clang_config;
    Ui::PathListConfigWidget* const m_system_list;
    Ui::PathListConfigWidget* const m_session_list;
    Ui::DetectCompilerPathsWidget* const m_compiler_paths;
    Ui::SessionPathsSetsWidget* const m_favorite_sets;
    Ui::CompletionSettings* const m_completion_settings;
    KProcess m_compiler_proc;
    QString m_output;
    QString m_error;

    struct IncludeSetInfo
    {
        KSharedConfigPtr m_config;
        QString m_file;
    };
    std::map<QString, IncludeSetInfo> m_include_sets;
};

}                                                           // namespace kate
// kate: hl C++11/Qt4;
