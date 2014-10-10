/**
 * \file
 *
 * \brief Class \c kate::PluginConfiguration (implementation)
 *
 * \date Fri Nov 23 11:25:46 MSK 2012 -- Initial design
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
#include "plugin_configuration.h"
#include "index/utils.h"

// Standard includes
#include <KDE/KConfigGroup>
#include <KDE/KDebug>
#include <KDE/KGlobal>
#include <KDE/KSharedConfig>
#include <KDE/KSharedConfigPtr>
#include <clang-c/Index.h>
#include <cassert>

namespace kate { namespace {
const QString GLOBAL_CONFIG_GROUP_NAME = "CppHelper";
const QString SESSION_GROUP_SUFFIX = ":cpp-helper";
const QString CONFIGURED_DIRS_ITEM = "ConfiguredDirs";
const QString SANITIZE_RULES_ITEM = "SanitizeRules";
const QString PCH_FILE_ITEM = "PCHFile";
const QString CLANG_CMDLINE_PARAMS_ITEM = "ClangCmdLineParams";
const QString USE_LT_GT_ITEM = "UseLtGt";
const QString USE_CWD_ITEM = "UseCwd";
const QString OPEN_FIRST_INCLUDE_ITEM = "OpenFirstInclude";
const QString USE_WILDCARD_SEARCH_ITEM = "UseWildcardSearch";
const QString APPEND_ON_IMPORT_ITEM = "AppendSanitizerRulesOnImport";
const QString MONITOR_DIRS_ITEM = "MonitorDirs";
const QString HIGHLIGHT_COMPLETIONS_ITEM = "HighlightCompletionItems";
const QString SANITIZE_COMPLETIONS_ITEM = "SanitizeCompletionItems";
const QString AUTO_COMPLETIONS_ITEM = "AutoCompletionItems";
const QString INCLUDE_MACROS_ITEM = "IncludeMacrosToCompletionResults";
const QString USE_PREFIX_COLUMN_ITEM = "UsePrefixColumn";
const QString IGNORE_EXTENSIONS_ITEM = "IgnoreExtensions";
const QString ENABLED_INDICES_ITEM = "EnabledIndices";

const QString SANITIZE_RULE_SEPARATOR = "<$replace-with$>";
}                                                           // anonymous namespace

void PluginConfiguration::readGlobalConfig()
{
    // Read global config
    kDebug(DEBUG_AREA) << "Reading global config: " << KGlobal::config()->name();
    KConfigGroup gcg(KGlobal::config(), GLOBAL_CONFIG_GROUP_NAME);
    auto dirs = gcg.readPathEntry(CONFIGURED_DIRS_ITEM, QStringList{});
    kDebug(DEBUG_AREA) << "Got global configured include path list: " << dirs;
    m_system_dirs = dirs;
    readSanitizeRulesFrom(gcg, true);
    kDebug(DEBUG_AREA) << "Got" << m_sanitize_rules.size() << "sanitize rules total";
    //
    Q_EMIT(systemDirsChanged());
}

void PluginConfiguration::readSessionConfig(KConfigBase* config, const QString& groupPrefix)
{
    kDebug(DEBUG_AREA) << "** CONFIG-MGR **: Reading session config: " << groupPrefix;
    // Read session config
    /// \todo Rename it!
    KConfigGroup scg(config, groupPrefix + SESSION_GROUP_SUFFIX);
    m_session_dirs = scg.readPathEntry(CONFIGURED_DIRS_ITEM, QStringList{});
    m_pch_header= scg.readPathEntry(PCH_FILE_ITEM, QString{});
    m_clang_params = scg.readPathEntry(CLANG_CMDLINE_PARAMS_ITEM, QString{});
    m_use_ltgt = scg.readEntry(USE_LT_GT_ITEM, QVariant{false}).toBool();
    m_use_cwd = scg.readEntry(USE_CWD_ITEM, QVariant{false}).toBool();
    m_ignore_ext = scg.readEntry(IGNORE_EXTENSIONS_ITEM, QStringList{});
    m_open_first = scg.readEntry(OPEN_FIRST_INCLUDE_ITEM, QVariant{false}).toBool();
    m_use_wildcard_search = scg.readEntry(USE_WILDCARD_SEARCH_ITEM, QVariant{false}).toBool();
    m_highlight_completions = scg.readEntry(HIGHLIGHT_COMPLETIONS_ITEM, QVariant{true}).toBool();
    m_sanitize_completions = scg.readEntry(SANITIZE_COMPLETIONS_ITEM, QVariant{true}).toBool();
    m_auto_completions = scg.readEntry(AUTO_COMPLETIONS_ITEM, QVariant{true}).toBool();
    m_include_macros = scg.readEntry(INCLUDE_MACROS_ITEM, QVariant{true}).toBool();
    m_use_prefix_column = scg.readEntry(USE_PREFIX_COLUMN_ITEM, QVariant{false}).toBool();
    m_append_sanitizer_rules_on_import = scg.readEntry(APPEND_ON_IMPORT_ITEM, QVariant{false}).toBool();
    //
    auto monitor_flags = scg.readEntry(MONITOR_DIRS_ITEM, QVariant{0}).toInt();
    if (monitor_flags < int(MonitorTargets::last__))
        m_monitor_flags = static_cast<MonitorTargets>(monitor_flags);
    else
        m_monitor_flags = MonitorTargets::nothing;
    //
    for (const auto& index : scg.readEntry(ENABLED_INDICES_ITEM, QStringList{}))
    {
        try
        {
            m_enabled_indices.insert(index::fromString(index));
        }
        catch (...)
        {
            /// \todo Make some spam?
            continue;
        }
    }
    m_config_dirty = false;

    kDebug(DEBUG_AREA) << "Got per session configured include path list: " << m_session_dirs;

    readGlobalConfig();

    Q_EMIT(sessionDirsChanged());
    Q_EMIT(dirWatchSettingsChanged());
}

void PluginConfiguration::writeSessionConfig(KConfigBase* config, const QString& groupPrefix)
{
    kDebug(DEBUG_AREA) << "** CONFIG-MGR **: Writing session config: " << groupPrefix;
    if (!m_config_dirty)
    {
        kDebug(DEBUG_AREA) << "Config isn't dirty!!!";
        readSessionConfig(config, groupPrefix);
        return;
    }

    // Write session config
    KConfigGroup scg(config, groupPrefix + SESSION_GROUP_SUFFIX);
    scg.writePathEntry(CONFIGURED_DIRS_ITEM, m_session_dirs);
    scg.writePathEntry(PCH_FILE_ITEM, m_pch_header.toLocalFile());
    scg.writeEntry(CLANG_CMDLINE_PARAMS_ITEM, m_clang_params);
    scg.writeEntry(IGNORE_EXTENSIONS_ITEM, m_ignore_ext);
    scg.writeEntry(MONITOR_DIRS_ITEM, int(m_monitor_flags));
    scg.writeEntry(AUTO_COMPLETIONS_ITEM, m_auto_completions);
    scg.writeEntry(HIGHLIGHT_COMPLETIONS_ITEM, m_highlight_completions);
    scg.writeEntry(INCLUDE_MACROS_ITEM, m_include_macros);
    scg.writeEntry(OPEN_FIRST_INCLUDE_ITEM, m_open_first);
    scg.writeEntry(SANITIZE_COMPLETIONS_ITEM, m_sanitize_completions);
    scg.writeEntry(USE_CWD_ITEM, m_use_cwd);
    scg.writeEntry(USE_LT_GT_ITEM, m_use_ltgt);
    scg.writeEntry(USE_PREFIX_COLUMN_ITEM, m_use_prefix_column);
    scg.writeEntry(USE_WILDCARD_SEARCH_ITEM, m_use_wildcard_search);
    scg.writeEntry(APPEND_ON_IMPORT_ITEM, m_append_sanitizer_rules_on_import);
    {
        auto enabled_indices = QStringList{};
        for (const auto& index : m_enabled_indices)
            enabled_indices << index::toString(index);
        scg.writeEntry(ENABLED_INDICES_ITEM, enabled_indices);
    }

    scg.sync();

    // Write global config
    KConfigGroup gcg(KGlobal::config(), GLOBAL_CONFIG_GROUP_NAME);
    gcg.writePathEntry(CONFIGURED_DIRS_ITEM, m_system_dirs);
    writeSanitizeRulesTo(gcg);
    gcg.sync();
    m_config_dirty = false;
}

void PluginConfiguration::setSessionDirs(QStringList& dirs)
{
    kDebug(DEBUG_AREA) << "Got session dirs: " << m_session_dirs;
    kDebug(DEBUG_AREA) << "... session dirs: " << dirs;
    if (m_session_dirs != dirs)
    {
        m_session_dirs.swap(dirs);
        m_config_dirty = true;
        Q_EMIT(sessionDirsChanged());
        Q_EMIT(dirWatchSettingsChanged());
    }
}

void PluginConfiguration::setSystemDirs(QStringList& dirs)
{
    kDebug(DEBUG_AREA) << "Got system dirs: " << m_system_dirs;
    kDebug(DEBUG_AREA) << "... system dirs: " << dirs;
    if (m_system_dirs != dirs)
    {
        m_system_dirs.swap(dirs);
        m_config_dirty = true;
        Q_EMIT(systemDirsChanged());
        Q_EMIT(dirWatchSettingsChanged());
    }
}

void PluginConfiguration::setIgnoreExtensions(QStringList& extensions)
{
    kDebug(DEBUG_AREA) << "Got ignore extensions: " << m_ignore_ext;
    if (m_ignore_ext != extensions)
    {
        m_ignore_ext.swap(extensions);
        m_config_dirty = true;
    }
}

void PluginConfiguration::setClangParams(const QString& params)
{
    if (m_clang_params != params)
    {
        m_clang_params = params;
        m_config_dirty = true;
        Q_EMIT(clangOptionsChanged());
        Q_EMIT(precompiledHeaderFileChanged());
    }
}

void PluginConfiguration::setPrecompiledHeaderFile(const KUrl& filename)
{
    if (m_pch_header != filename)
    {
        m_pch_header = filename;
        m_config_dirty = true;
        Q_EMIT(precompiledHeaderFileChanged());
    }
}

/// \todo Set dirty flag only if smth really has changed
void PluginConfiguration::setIndexState(const QString& uuid_str, const bool flag)
{
    auto uuid = index::fromString(uuid_str);
    if (flag)
        m_enabled_indices.insert(uuid);
    else
        m_enabled_indices.erase(uuid);
    m_config_dirty = true;
}

void PluginConfiguration::setMonitorTargets(const MonitorTargets tgt)
{
    if (m_monitor_flags != tgt)
    {
        m_monitor_flags = tgt;
        m_config_dirty = true;
        Q_EMIT(dirWatchSettingsChanged());
    }
}

clang::compiler_options PluginConfiguration::formCompilerOptions() const
{
    auto session_dirs = sessionDirs();
    auto system_dirs = systemDirs();
    QStringList options;
    // reserve space for at least known options count
    options.reserve(system_dirs.size() + session_dirs.size());
    // 1) split configured aux options and append to collected
    for (const QString& o : clangParams().split(QRegExp{"\\s+"}, QString::SkipEmptyParts))
        options.push_back(o);
    // 2) add configured system dirs as -I options
    for (const QString& dir : system_dirs)
        options.push_back("-I" + dir);
    // 3) add configured session dirs as -I options
    for (const QString& dir : session_dirs)
        options.push_back("-I" + dir);

    return clang::compiler_options{options};
}

unsigned PluginConfiguration::completionFlags() const
{
    auto flags = clang_defaultCodeCompleteOptions() | CXCodeComplete_IncludeBriefComments;
    if (includeMacros())
    {
        kDebug(DEBUG_AREA) << "Allow preprocessor MACROS in completion results";
        flags |= CXCodeComplete_IncludeMacros;
    }
    return flags;
}

/**
 * \param grp group to read rules
 * \param need_clear should all current rule must be removed before
 */
void PluginConfiguration::readSanitizeRulesFrom(const KConfigGroup& grp, const bool need_clear)
{
    if (need_clear)
        m_sanitize_rules.clear();

    // Read sanitize rules from serializable form
    auto rules = grp.readPathEntry(SANITIZE_RULES_ITEM, QStringList{});
    for (auto&& rule : rules)
    {
        auto l = rule.split(SANITIZE_RULE_SEPARATOR);
        auto find = QString{};
        auto replace = QString{};
        switch (l.size())
        {
            case 2:
                replace.swap(l[1]);
            case 1:
                find.swap(l[0]);
                break;
            default:
            kWarning() << "Invalid sanitize rule ignored: " << rule;
        }
        kDebug(DEBUG_AREA) << "Got sanitize rule: find =" << find << ", replace =" << replace;
        if (!find.isEmpty())
        {
            auto find_regex = QRegExp{find};
            if (find_regex.isValid())
                m_sanitize_rules.emplace_back(std::move(find_regex), std::move(replace));
            else
                kWarning() << "Invalid sanitize rule ignored: " << rule;
        }
    }
}

/**
 * This function used to write configured rules to a given \c KConfigGroup.
 * It happens on `kate` start and on sanitize rules export.
 * 
 * \param grp a group to write rules to
 *
 * \todo Use smth more human readable to export sanitize rules? XML?
 */
void PluginConfiguration::writeSanitizeRulesTo(KConfigGroup& grp)
{
    // Transform sanitize rules into a serializable list of strings
    QStringList rules;
    for (auto&& rule : sanitizeRules())
    {
        auto r = rule.first.pattern();
        if (!rule.first.isValid())                          // Ignore invalid regular expressions
        {
            kWarning() << "Ignore invalid sanitize regex: " << r;
            continue;
        }
        if (r.isEmpty())                                    // Ignore rules w/ empty find part
        {
            kWarning() << "Ignore invalid sanitize rule: " << rule.second;
            continue;
        }
        if (!rule.second.isEmpty())
            r += SANITIZE_RULE_SEPARATOR + rule.second;
        rules << r;
    }
    grp.writePathEntry(SANITIZE_RULES_ITEM, rules);
}

}                                                           // namespace kate
