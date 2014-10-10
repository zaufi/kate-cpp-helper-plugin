/**
 * \file
 *
 * \brief Class \c kate::PluginConfiguration (interface)
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

#pragma once

// Project specific includes
#include "clang/compiler_options.h"

// Standard includes
#include <boost/uuid/uuid.hpp>
#include <KDE/KConfigBase>
#include <KDE/KUrl>
#include <QtCore/QRegExp>
#include <QtCore/QStringList>
#include <set>
#include <utility>
#include <vector>

namespace kate {

/**
 * \brief Class to manage configuration data of the plug-in
 *
 * [More detailed description here]
 *
 */
class PluginConfiguration : public QObject
{
    Q_OBJECT

public:
    enum class MonitorTargets
    {
        nothing
      , systemDirs
      , sessionDirs
      , both
      , last__
    };

    typedef std::vector<std::pair<QRegExp, QString>> sanitize_rules_list_type;

    /// \name Accessors
    //@{
    const sanitize_rules_list_type& sanitizeRules() const;
    const QStringList& systemDirs() const;
    const QStringList& sessionDirs() const;
    const QStringList& ignoreExtensions() const;
    const std::set<boost::uuids::uuid>& enabledIndices() const;
    const KUrl& precompiledHeaderFile() const;
    const KUrl& pchFile() const;
    const QString& clangParams() const;
    MonitorTargets monitorTargets() const;
    bool autoCompletions() const;
    bool highlightCompletions() const;
    bool includeMacros() const;
    bool sanitizeCompletions() const;
    bool shouldOpenFirstInclude() const;
    bool useCwd() const;
    bool useLtGt() const;
    bool usePrefixColumn() const;
    bool useWildcardSearch() const;
    unsigned completionFlags() const;
    bool appendOnImport() const;
    //@}

    /// \name Modifiers
    //@{
    void setSanitizeRules(sanitize_rules_list_type&&);
    void setSystemDirs(QStringList&);
    void setSessionDirs(QStringList&);
    void setIgnoreExtensions(QStringList&);
    void setPrecompiledHeaderFile(const KUrl&);
    void setPrecompiledFile(const KUrl&);
    void setClangParams(const QString&);
    void setMonitorTargets(MonitorTargets);
    void setAutoCompletions(bool);
    void setHighlightCompletions(bool);
    void setIncludeMacros(bool);
    void setOpenFirst(bool);
    void setSanitizeCompletions(bool);
    void setUseCwd(bool);
    void setUseLtGt(bool);
    void setUsePrefixColumn(bool);
    void setUseWildcardSearch(bool);
    void setAppendOnImport(bool);
    //@}

    void readSessionConfig(KConfigBase*, const QString&);
    void writeSessionConfig(KConfigBase*, const QString&);
    void readGlobalConfig();                                ///< Read global config

    clang::compiler_options formCompilerOptions() const;

    /// Read sanitize rules from a given config group
    void readSanitizeRulesFrom(const KConfigGroup&, const bool);
    /// Write sanitize rules to a given config group
    void writeSanitizeRulesTo(KConfigGroup&);

public Q_SLOTS:
    void setIndexState(const QString&, bool);

Q_SIGNALS:
    void clangOptionsChanged();
    void dirWatchSettingsChanged();
    void precompiledHeaderFileChanged();
    void sessionDirsChanged();
    void systemDirsChanged();

private:
    sanitize_rules_list_type m_sanitize_rules;
    QStringList m_system_dirs;
    QStringList m_session_dirs;
    QStringList m_ignore_ext;
    std::set<boost::uuids::uuid> m_enabled_indices;
    KUrl m_pch_header;
    KUrl m_pch_file;
    QString m_clang_params;
    MonitorTargets m_monitor_flags = {MonitorTargets::nothing};
    bool m_auto_completions = {false};
    bool m_config_dirty = {false};
    bool m_highlight_completions = {true};                  ///< Try to highlight code completion results
    bool m_include_macros = {true};                         ///< Include MACROS to completion results
    bool m_open_first = {false};
    /// Use sanitize rules to cleanup code completion results
    bool m_sanitize_completions = {true};
    bool m_use_cwd = {false};                               ///< Use current document'd dir to find \c #include
    /// If \c true <em>Copy #include</em> action would put filename into \c '<' and \c '>' instead of \c '"'
    bool m_use_ltgt = {true};
    /// Use \em prefix column for result type or item kind
    bool m_use_prefix_column = {true};
    bool m_use_wildcard_search = {false};
    /// Append (\c true) or replace (\c false) sanitizer rules on \e import action
    bool m_append_sanitizer_rules_on_import = {false};
};

inline auto PluginConfiguration::sanitizeRules() const -> const sanitize_rules_list_type&
{
    return m_sanitize_rules;
}
inline const QStringList& PluginConfiguration::systemDirs() const
{
    return m_system_dirs;
}

inline const QStringList& PluginConfiguration::sessionDirs() const
{
    return m_session_dirs;
}
inline const QStringList& PluginConfiguration::ignoreExtensions() const
{
    return m_ignore_ext;
}
inline const std::set<boost::uuids::uuid>& PluginConfiguration::enabledIndices() const
{
    return m_enabled_indices;
}
inline const KUrl& PluginConfiguration::precompiledHeaderFile() const
{
    return m_pch_header;
}
inline const KUrl& PluginConfiguration::pchFile() const
{
    return m_pch_file;
}
inline const QString& PluginConfiguration::clangParams() const
{
    return m_clang_params;
}
inline bool PluginConfiguration::useLtGt() const
{
    return m_use_ltgt;
}
inline bool PluginConfiguration::useCwd() const
{
    return m_use_cwd;
}
inline bool PluginConfiguration::shouldOpenFirstInclude() const
{
    return m_open_first;
}
inline bool PluginConfiguration::useWildcardSearch() const
{
    return m_use_wildcard_search;
}
inline bool PluginConfiguration::appendOnImport() const
{
    return m_append_sanitizer_rules_on_import;
}
inline auto PluginConfiguration::monitorTargets() const -> MonitorTargets
{
    return m_monitor_flags;
}
inline bool PluginConfiguration::sanitizeCompletions() const
{
    return m_sanitize_completions;
}
inline bool PluginConfiguration::highlightCompletions() const
{
    return m_highlight_completions;
}
inline bool PluginConfiguration::autoCompletions() const
{
    return m_auto_completions;
}
inline bool PluginConfiguration::includeMacros() const
{
    return m_include_macros;
}

inline bool PluginConfiguration::usePrefixColumn() const
{
    return m_use_prefix_column;
}

inline void PluginConfiguration::setPrecompiledFile(const KUrl& file)
{
    m_pch_file = file;
}


/**
 * \todo Add overload to accept a sequence of pairs of \c QString,
 * and move regex validation code here avoiding duplicates outside of
 * this class.
 */
inline void PluginConfiguration::setSanitizeRules(sanitize_rules_list_type&& rules)
{
    m_sanitize_rules = std::move(rules);
    m_config_dirty = true;
}

inline void PluginConfiguration::setAppendOnImport(const bool state)
{
    m_append_sanitizer_rules_on_import = state;
    m_config_dirty = true;
}

inline void PluginConfiguration::setUseLtGt(const bool state)
{
    m_use_ltgt = state;
    m_config_dirty = true;
}

inline void PluginConfiguration::setUseCwd(const bool state)
{
    m_use_cwd = state;
    m_config_dirty = true;
}

inline void PluginConfiguration::setOpenFirst(const bool state)
{
    m_open_first = state;
    m_config_dirty = true;
}

inline void PluginConfiguration::setUseWildcardSearch(const bool state)
{
    m_use_wildcard_search = state;
    m_config_dirty = true;
}

inline void PluginConfiguration::setHighlightCompletions(const bool state)
{
    m_highlight_completions = state;
    m_config_dirty = true;
}

inline void PluginConfiguration::setSanitizeCompletions(const bool state)
{
    m_sanitize_completions = state;
    m_config_dirty = true;
}

inline void PluginConfiguration::setAutoCompletions(const bool state)
{
    m_auto_completions = state;
    m_config_dirty = true;
}

inline void PluginConfiguration::setIncludeMacros(const bool state)
{
    m_include_macros = state;
    m_config_dirty = true;
}

inline void PluginConfiguration::setUsePrefixColumn(const bool state)
{
    m_use_prefix_column = state;
    m_config_dirty = true;
}

}                                                           // namespace kate
