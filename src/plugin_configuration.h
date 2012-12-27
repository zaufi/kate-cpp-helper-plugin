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

#ifndef __SRC__PLUGIN_CONFIGURATION_H__
# define __SRC__PLUGIN_CONFIGURATION_H__

// Project specific includes

// Standard includes
# include <KConfigBase>
# include <KUrl>
# include <QtCore/QStringList>

namespace kate {

/**
 * \brief Class to manage configuration data of the plugin
 *
 * [More detailed description here]
 *
 */
class PluginConfiguration
  : public QObject
{
    Q_OBJECT

public:
    PluginConfiguration()
      : m_monitor_flags(0)
      , m_use_ltgt(true)
      , m_config_dirty(false)
      , m_open_first(false)
      , m_use_wildcard_search(false)
      , m_highlight_completions(true)
      , m_sanitize_completions(true)
      , m_auto_completions(true)
    {}

    /// \name Accessors
    //@{
    const QStringList& sessionDirs() const;
    const QStringList& systemDirs() const;
    const KUrl& precompiledHeaderFile() const;
    const KUrl& pchFile() const;
    const QString& clangParams() const;
    bool useLtGt() const;
    bool useCwd() const;
    bool shouldOpenFirstInclude() const;
    bool useWildcardSearch() const;
    int what_to_monitor() const;
    bool sanitizeCompletions() const;
    bool highlightCompletions() const;
    bool autoCompletions() const;

    //@}

    /// \name Modifiers
    //@{
    void setSessionDirs(QStringList&);
    void setGlobalDirs(QStringList&);
    void setClangParams(const QString&);
    void setPrecompiledHeaderFile(const KUrl&);
    void setPrecompiledFile(const KUrl&);
    void setUseLtGt(const bool);
    void setUseCwd(const bool);
    void setOpenFirst(const bool);
    void setUseWildcardSearch(const bool);
    void setWhatToMonitor(const int);
    void setSanitizeCompletions(const bool);
    void setHighlightCompletions(const bool);
    void setAutoCompletions(const bool);
    //@}

    void readSessionConfig(KConfigBase*, const QString&);
    void writeSessionConfig(KConfigBase*, const QString&);
    void readConfig();                                      ///< Read global config

    QStringList formCompilerOptions() const;

Q_SIGNALS:
    void dirWatchSettingsChanged();
    void sessionDirsChanged();
    void systemDirsChanged();
    void precompiledHeaderFileChanged();
    void clangOptionsChanged();

private:
    QStringList m_system_dirs;
    QStringList m_session_dirs;
    KUrl m_pch_header;
    KUrl m_pch_file;
    QString m_clang_params;
    int m_monitor_flags;
    /// If \c true <em>Copy #include</em> action would put filename into \c '<' and \c '>'
    /// instead of \c '"'
    bool m_use_ltgt;
    bool m_use_cwd;
    bool m_config_dirty;
    bool m_open_first;
    bool m_use_wildcard_search;
    bool m_highlight_completions;
    bool m_sanitize_completions;
    bool m_auto_completions;
};

inline const QStringList& PluginConfiguration::sessionDirs() const
{
    return m_session_dirs;
}
inline const QStringList& PluginConfiguration::systemDirs() const
{
    return m_system_dirs;
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
inline int PluginConfiguration::what_to_monitor() const
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

inline void PluginConfiguration::setPrecompiledFile(const KUrl& file)
{
    m_pch_file = file;
}

}                                                           // namespace kate
#endif                                                      // __SRC__PLUGIN_CONFIGURATION_H__
// kate: hl C++11/Qt4;
