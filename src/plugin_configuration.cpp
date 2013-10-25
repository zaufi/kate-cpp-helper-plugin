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
const QString SANITIZE_RULE_SEPARATOR = "<$replace-with$>";
}                                                           // anonymous namespace

QString PluginConfiguration::makeSureUnderlaidConfigsInitialized(
    KConfigBase* config
  , const QString& groupPrefix
  )
{
    /// \attention Is there OTHER (legal) way to get session config file?
    auto* downcasted_config = dynamic_cast<KConfig*>(config);
    assert("Sanity check" && downcasted_config);
    auto session_config_file = downcasted_config->name();
    auto session_config = KSharedConfig::openConfig(session_config_file, KConfig::SimpleConfig);

    m_session.reset(
        new SessionPluginConfiguration{session_config, groupPrefix + SESSION_GROUP_SUFFIX}
      );

    m_system.reset(new SystemPluginConfiguration{});
    return session_config_file;
}

void PluginConfiguration::readSessionConfig(KConfigBase* config, const QString& groupPrefix)
{
    auto session_config_file = makeSureUnderlaidConfigsInitialized(config, groupPrefix);

    kDebug(DEBUG_AREA) << "** CONFIG-MGR **: Reading session config: "
      << session_config_file << "[" << groupPrefix << "]";

    kDebug(DEBUG_AREA) << "Got per session configured include path list: " << sessionDirs();
    kDebug(DEBUG_AREA) << "Got global configured include path list: " << systemDirs();

    // Read sanitize rules from serializable form
    m_sanitize_rules.clear();
    auto rules = m_system->sanitizeRules();
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

    Q_EMIT(sessionDirsChanged());
    Q_EMIT(systemDirsChanged());
    Q_EMIT(dirWatchSettingsChanged());
}

void PluginConfiguration::writeSessionConfig(KConfigBase* config, const QString& groupPrefix)
{
    auto session_config_file = makeSureUnderlaidConfigsInitialized(config, groupPrefix);

    kDebug(DEBUG_AREA) << "** CONFIG-MGR **: Writing session config: "
      << session_config_file << "[" << groupPrefix << "]";

    config->sync();
    m_session->writeConfig();
    m_system->writeConfig();
    config->sync();
}

void PluginConfiguration::setIndexState(const QString& index_name, const bool flag)
{
    auto enabled_indices = m_session->enabledIndices();
    auto idx = enabled_indices.indexOf(index_name);
    if (idx == -1 && flag)
        enabled_indices << index_name;
    else if (idx != -1 && !flag)
        enabled_indices.removeAt(idx);
    m_session->setEnabledIndices(enabled_indices);
}

void PluginConfiguration::renameIndex(const QString& old_name, const QString& new_name)
{
    auto enabled_indices = m_session->enabledIndices();
    auto idx = enabled_indices.indexOf(old_name);
    if (idx != -1)
    {
        enabled_indices.removeAt(idx);
        enabled_indices << new_name;
    }
    m_session->setEnabledIndices(enabled_indices);
}

/**
 * \todo Add overload to accept a sequence of pairs of \c QString,
 * and move regex validation code here avoiding duplicates outside of
 * this class.
 */
void PluginConfiguration::setSanitizeRules(sanitize_rules_list_type&& rules)
{
    m_sanitize_rules = std::move(rules);
    // Transform sanitize rules into a serializable list of strings
    QStringList transformed_rules;
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
        transformed_rules << r;
    }
    m_system->setSanitizeRules(transformed_rules);
}

clang::compiler_options PluginConfiguration::formCompilerOptions() const
{
    auto session_dirs = sessionDirs();
    auto system_dirs = systemDirs();
    QStringList options;
    // reserve space for at least known options count
    options.reserve(system_dirs.size() + session_dirs.size());
    // 1) split configured aux options and append to collected
    for (const QString& o : clangParams().split(QRegExp("\\s+"), QString::SkipEmptyParts))
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

}                                                           // namespace kate
// kate: hl C++11/Qt4;
