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
#include <src/plugin_configuration.h>

// Standard includes
#include <clang-c/Index.h>
#include <KConfigGroup>
#include <KDebug>
#include <KGlobal>
#include <KSharedConfig>
#include <KSharedConfigPtr>
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
const QString MONITOR_DIRS_ITEM = "MonitorDirs";
const QString HIGHLIGHT_COMPLETIONS_ITEM = "HighlightCompletionItems";
const QString SANITIZE_COMPLETIONS_ITEM = "SanitizeCompletionItems";
const QString AUTO_COMPLETIONS_ITEM = "AutoCompletionItems";
const QString INCLUDE_MACROS_ITEM = "IncludeMacrosToCompletionResults";
const QString IGNORE_EXTENSIONS_ITEM = "IgnoreExtensions";

const QString SANITIZE_RULE_SEPARATOR = "<$replace-with$>";
}                                                           // anonymous namespace

void PluginConfiguration::readConfig()
{
    // Read global config
    kDebug(DEBUG_AREA) << "Reading global config: " << KGlobal::config()->name();
    KConfigGroup gcg(KGlobal::config(), GLOBAL_CONFIG_GROUP_NAME);
    auto dirs = gcg.readPathEntry(CONFIGURED_DIRS_ITEM, QStringList());
    kDebug(DEBUG_AREA) << "Got global configured include path list: " << dirs;
    m_system_dirs = dirs;
    // Read sanitize rules from serializable form
    m_sanitize_rules.clear();
    auto rules = gcg.readPathEntry(SANITIZE_RULES_ITEM, QStringList());
    for (auto&& rule : rules)
    {
        auto l = rule.split(SANITIZE_RULE_SEPARATOR);
        QString find;
        QString replace;
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
            auto find_regex = QRegExp(find);
            if (find_regex.isValid())
                m_sanitize_rules.emplace_back(std::move(find_regex), std::move(replace));
            else
                kWarning() << "Invalid sanitize rule ignored: " << rule;
        }
    }
    kDebug(DEBUG_AREA) << "Got" << m_sanitize_rules.size() << "sanitize rules total";
    //
    Q_EMIT(sessionDirsChanged());
    Q_EMIT(systemDirsChanged());
    Q_EMIT(dirWatchSettingsChanged());
}

void PluginConfiguration::readSessionConfig(KConfigBase* config, const QString& groupPrefix)
{
    kDebug(DEBUG_AREA) << "** CONFIG-MGR **: Reading session config: " << groupPrefix;
    // Read session config
    /// \todo Rename it!
    KConfigGroup scg(config, groupPrefix + SESSION_GROUP_SUFFIX);
    m_session_dirs = scg.readPathEntry(CONFIGURED_DIRS_ITEM, QStringList());
    m_pch_header= scg.readPathEntry(PCH_FILE_ITEM, QString());
    m_clang_params = scg.readPathEntry(CLANG_CMDLINE_PARAMS_ITEM, QString());
    m_use_ltgt = scg.readEntry(USE_LT_GT_ITEM, QVariant(false)).toBool();
    m_use_cwd = scg.readEntry(USE_CWD_ITEM, QVariant(false)).toBool();
    m_ignore_ext = scg.readEntry(IGNORE_EXTENSIONS_ITEM, QStringList());
    m_open_first = scg.readEntry(OPEN_FIRST_INCLUDE_ITEM, QVariant(false)).toBool();
    m_use_wildcard_search = scg.readEntry(USE_WILDCARD_SEARCH_ITEM, QVariant(false)).toBool();
    m_highlight_completions = scg.readEntry(HIGHLIGHT_COMPLETIONS_ITEM, QVariant(true)).toBool();
    m_sanitize_completions = scg.readEntry(SANITIZE_COMPLETIONS_ITEM, QVariant(true)).toBool();
    m_auto_completions = scg.readEntry(AUTO_COMPLETIONS_ITEM, QVariant(true)).toBool();
    m_include_macros = scg.readEntry(INCLUDE_MACROS_ITEM, QVariant(true)).toBool();
    m_monitor_flags = scg.readEntry(MONITOR_DIRS_ITEM, QVariant(0)).toInt();
    m_config_dirty = false;

    kDebug(DEBUG_AREA) << "Got per session configured include path list: " << m_session_dirs;

    readConfig();
}

void PluginConfiguration::writeSessionConfig(KConfigBase* config, const QString& groupPrefix)
{
    kDebug(DEBUG_AREA) << "** CONFIG-MGR **: Writing session config: " << groupPrefix;
    if (!m_config_dirty)
    {
        /// \todo Maybe I don't understand smth, but rally strange things r going on here:
        /// after plugin gets enabled, \c writeSessionConfig() will be called \b BEFORE
        /// any attempt to read configuration...
        /// The only thing came into my mind that it is attempt to initialize config w/ default
        /// values... but in that case everything stored before get lost!
        kDebug(DEBUG_AREA) << "Config isn't dirty!!!";
        readSessionConfig(config, groupPrefix);
        return;
    }

    // Write session config
    kDebug(DEBUG_AREA) << "Write per session configured include path list: " << m_session_dirs;
    KConfigGroup scg(config, groupPrefix + SESSION_GROUP_SUFFIX);
    scg.writePathEntry(CONFIGURED_DIRS_ITEM, m_session_dirs);
    scg.writeEntry(PCH_FILE_ITEM, m_pch_header);
    scg.writeEntry(CLANG_CMDLINE_PARAMS_ITEM, m_clang_params);
    scg.writeEntry(USE_LT_GT_ITEM, QVariant(m_use_ltgt));
    scg.writeEntry(USE_CWD_ITEM, QVariant(m_use_cwd));
    scg.writeEntry(IGNORE_EXTENSIONS_ITEM, m_ignore_ext);
    scg.writeEntry(OPEN_FIRST_INCLUDE_ITEM, QVariant(m_open_first));
    scg.writeEntry(USE_WILDCARD_SEARCH_ITEM, QVariant(m_use_wildcard_search));
    scg.writeEntry(HIGHLIGHT_COMPLETIONS_ITEM, QVariant(m_highlight_completions));
    scg.writeEntry(SANITIZE_COMPLETIONS_ITEM, QVariant(m_sanitize_completions));
    scg.writeEntry(AUTO_COMPLETIONS_ITEM, QVariant(m_auto_completions));
    scg.writeEntry(INCLUDE_MACROS_ITEM, QVariant(m_include_macros));
    scg.writeEntry(MONITOR_DIRS_ITEM, QVariant(m_monitor_flags));
    scg.sync();
    // Write global config
    kDebug(DEBUG_AREA) << "Write global configured include path list: " << m_system_dirs;
    KConfigGroup gcg(KGlobal::config(), GLOBAL_CONFIG_GROUP_NAME);
    gcg.writePathEntry(CONFIGURED_DIRS_ITEM, m_system_dirs);
    // Transform sanitize rules into a serializable list of strings
    QStringList rules;
    for (auto&& rule : m_sanitize_rules)
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
    gcg.writePathEntry(SANITIZE_RULES_ITEM, rules);
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
        kDebug(DEBUG_AREA) << "** set config to `dirty' state!! **";
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
        kDebug(DEBUG_AREA) << "** set config to `dirty' state!! **";
    }
}

void PluginConfiguration::setIgnoreExtensions(QStringList& extensions)
{
    kDebug(DEBUG_AREA) << "Got ignore extensions: " << m_ignore_ext;
    if (m_ignore_ext != extensions)
    {
        m_ignore_ext.swap(extensions);
        m_config_dirty = true;
        kDebug(DEBUG_AREA) << "** set config to `dirty' state!! **";
    }
}

void PluginConfiguration::setClangParams(const QString& params)
{
    if (m_clang_params != params)
    {
        m_clang_params = params;
        m_config_dirty = true;
        kDebug(DEBUG_AREA) << "** set config to `dirty' state!! **";
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
        kDebug(DEBUG_AREA) << "** set config to `dirty' state!! **";
        Q_EMIT(precompiledHeaderFileChanged());
    }
}

void PluginConfiguration::setUseLtGt(const bool state)
{
    m_use_ltgt = state;
    m_config_dirty = true;
    kDebug(DEBUG_AREA) << "** set config to `dirty' state!! **";
}

void PluginConfiguration::setUseCwd(const bool state)
{
    m_use_cwd = state;
    m_config_dirty = true;
    kDebug(DEBUG_AREA) << "** set config to `dirty' state!! **";
}

void PluginConfiguration::setOpenFirst(const bool state)
{
    m_open_first = state;
    m_config_dirty = true;
    kDebug(DEBUG_AREA) << "** set config to `dirty' state!! **";
}

void PluginConfiguration::setUseWildcardSearch(const bool state)
{
    m_use_wildcard_search = state;
    m_config_dirty = true;
    kDebug(DEBUG_AREA) << "** set config to `dirty' state!! **";
}

void PluginConfiguration::setHighlightCompletions(const bool state)
{
    m_highlight_completions = state;
    m_config_dirty = true;
    kDebug(DEBUG_AREA) << "** set config to `dirty' state!! **";
}

void PluginConfiguration::setSanitizeCompletions(const bool state)
{
    m_sanitize_completions = state;
    m_config_dirty = true;
    kDebug(DEBUG_AREA) << "** set config to `dirty' state!! **";
}

void PluginConfiguration::setAutoCompletions(const bool state)
{
    m_auto_completions = state;
    m_config_dirty = true;
    kDebug(DEBUG_AREA) << "** set config to `dirty' state!! **";
}

void PluginConfiguration::setIncludeMacros(const bool state)
{
    m_include_macros = state;
    m_config_dirty = true;
    kDebug(DEBUG_AREA) << "** set config to `dirty' state!! **";
}

/**
 * \todo Add overload to accept a sequence of pairs of \c QString,
 * and move regex validation code here avoiding duplicates outside of
 * this class.
 */
void PluginConfiguration::setSanitizeRules(sanitize_rules_list_type&& rules)
{
    m_sanitize_rules = std::move(rules);
    m_config_dirty = true;
    kDebug(DEBUG_AREA) << "** set config to `dirty' state!! **";
}

void PluginConfiguration::setWhatToMonitor(const int tgt)
{
    if (m_monitor_flags != tgt)
    {
        assert("Sanity check" && 0 <= tgt && tgt < 4);
        m_monitor_flags = tgt;
        m_config_dirty = true;
        Q_EMIT(dirWatchSettingsChanged());
    }
    kDebug(DEBUG_AREA) << "** set config to `dirty' state!! **";
}

QStringList PluginConfiguration::formCompilerOptions() const
{
    QStringList options;
    // reserve space for at least known options count
    options.reserve(systemDirs().size() + sessionDirs().size());
    // 1) split configured aux options and append to collected
    for (const QString& o : clangParams().split(QRegExp("\\s+"), QString::SkipEmptyParts))
        options.push_back(o);
    // 2) add configured system dirs as -I options
    for (const QString& dir : systemDirs())
        options.push_back("-I" + dir);
    // 3) add configured session dirs as -I options
    for (const QString& dir : sessionDirs())
        options.push_back("-I" + dir);

    return options;
}

unsigned PluginConfiguration::completionFlags() const
{
    auto flags = static_cast<unsigned>(CXCodeComplete_IncludeBriefComments);
    if (includeMacros())
    {
        kDebug(DEBUG_AREA) << "Allow preprocessor MACROS in completion results";
        flags |= CXCodeComplete_IncludeMacros;
    }
    return flags;
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
