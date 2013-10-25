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
#include <src/clang/compiler_options.h>
#include <src/cpp_helper_plugin_config.h>
#include <src/cpp_helper_plugin_session_config.h>

// Standard includes
#include <KDE/KUrl>
#include <QtCore/QString>
#include <QtCore/QRegExp>
#include <memory>
#include <vector>
#include <utility>

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
    typedef std::vector<std::pair<QRegExp, QString>> sanitize_rules_list_type;

    PluginConfiguration()
    {}

    /// \name Accessors
    //@{
    QStringList systemDirs() const;
    const sanitize_rules_list_type& sanitizeRules() const;

    QStringList sessionDirs() const;
    QStringList enabledIndices() const;
    QStringList ignoreExtensions() const;
    KUrl precompiledHeaderFile() const;
    KUrl pchFile() const;
    QString clangParams() const;
    SessionPluginConfiguration::EnumMonitorTargets::type whatToMonitor() const;
    bool useLtGt() const;
    bool useCwd() const;
    bool shouldOpenFirstInclude() const;
    bool useWildcardSearch() const;
    bool sanitizeCompletions() const;
    bool highlightCompletions() const;
    bool autoCompletions() const;
    bool includeMacros() const;
    bool usePrefixColumn() const;
    unsigned completionFlags() const;
    //@}

    /// \name Modifiers
    //@{
    void setSessionDirs(const QStringList&);
    void setSystemDirs(const QStringList&);
    void setIgnoreExtensions(const QStringList&);
    void setClangParams(const QString&);
    void setPrecompiledHeaderFile(const KUrl&);
    void setPrecompiledFile(const KUrl&);
    void setUseLtGt(bool);
    void setUseCwd(bool);
    void setOpenFirst(bool);
    void setUseWildcardSearch(bool);
    void setWhatToMonitor(SessionPluginConfiguration::EnumMonitorTargets::type);
    void setSanitizeCompletions(bool);
    void setHighlightCompletions(bool);
    void setAutoCompletions(bool);
    void setSanitizeRules(sanitize_rules_list_type&&);
    void setIncludeMacros(bool);
    void setUsePrefixColumn(bool);
    //@}

    void readSessionConfig(KConfigBase*, const QString&);
    void writeSessionConfig(KConfigBase*, const QString&);

    clang::compiler_options formCompilerOptions() const;

public Q_SLOTS:
    void setIndexState(const QString&, bool);
    void renameIndex(const QString&, const QString&);

Q_SIGNALS:
    void dirWatchSettingsChanged();
    void sessionDirsChanged();
    void systemDirsChanged();
    void precompiledHeaderFileChanged();
    void compilerOptionsChanged();

private:
    QString makeSureUnderlaidConfigsInitialized(KConfigBase*, const QString&);

    std::unique_ptr<SystemPluginConfiguration> m_system;
    std::unique_ptr<SessionPluginConfiguration> m_session;
    sanitize_rules_list_type m_sanitize_rules;
    KUrl m_pch_file;
    bool m_config_dirty;
};

inline QStringList PluginConfiguration::systemDirs() const
{
    return m_system->systemDirs();
}
inline auto PluginConfiguration::sanitizeRules() const -> const sanitize_rules_list_type&
{
    return m_sanitize_rules;
}

inline QStringList PluginConfiguration::sessionDirs() const
{
    return m_session->sessionDirs();
}
inline QStringList PluginConfiguration::ignoreExtensions() const
{
    return m_session->ignoreExtensions();
}
inline QStringList PluginConfiguration::enabledIndices() const
{
    return m_session->enabledIndices();
}
inline KUrl PluginConfiguration::precompiledHeaderFile() const
{
    return m_session->pchHeader();
}
inline KUrl PluginConfiguration::pchFile() const
{
    return m_pch_file;
}
inline QString PluginConfiguration::clangParams() const
{
    return m_session->clangParams();
}
inline bool PluginConfiguration::useLtGt() const
{
    return m_session->useLtGt();
}
inline bool PluginConfiguration::useCwd() const
{
    return m_session->useCwd();
}
inline bool PluginConfiguration::shouldOpenFirstInclude() const
{
    return m_session->openFirst();
}
inline bool PluginConfiguration::useWildcardSearch() const
{
    return m_session->useWildcardSearch();
}
inline SessionPluginConfiguration::EnumMonitorTargets::type PluginConfiguration::whatToMonitor() const
{
    return SessionPluginConfiguration::EnumMonitorTargets::type(m_session->monitorTargets());
}
inline bool PluginConfiguration::highlightCompletions() const
{
    return m_session->highlightCompletions();
}
inline bool PluginConfiguration::sanitizeCompletions() const
{
    return m_session->sanitizeCompletions();
}
inline bool PluginConfiguration::autoCompletions() const
{
    return m_session->autoCompletions();
}
inline bool PluginConfiguration::includeMacros() const
{
    return m_session->includeMacros();
}

inline bool PluginConfiguration::usePrefixColumn() const
{
    return m_session->usePrefixColumn();
}
inline void PluginConfiguration::setPrecompiledFile(const KUrl& file)
{
    m_pch_file = file;
}

inline void PluginConfiguration::setSystemDirs(const QStringList& dirs)
{
    m_system->setSystemDirs(dirs);
    Q_EMIT(systemDirsChanged());
    Q_EMIT(dirWatchSettingsChanged());
}

inline void PluginConfiguration::setSessionDirs(const QStringList& dirs)
{
    m_session->setSessionDirs(dirs);
    Q_EMIT(sessionDirsChanged());
    Q_EMIT(dirWatchSettingsChanged());
}
inline void PluginConfiguration::setIgnoreExtensions(const QStringList& exts)
{
    m_session->setIgnoreExtensions(exts);
}
inline void PluginConfiguration::setClangParams(const QString& params)
{
    m_session->setClangParams(params);
    Q_EMIT(compilerOptionsChanged());
    Q_EMIT(precompiledHeaderFileChanged());
}
inline void PluginConfiguration::setPrecompiledHeaderFile(const KUrl& file)
{
    m_session->setPchHeader(file);
    Q_EMIT(precompiledHeaderFileChanged());
}
inline void PluginConfiguration::setUseLtGt(const bool f)
{
    m_session->setUseLtGt(f);
}
inline void PluginConfiguration::setUseCwd(const bool f)
{
    m_session->setUseCwd(f);
}
inline void PluginConfiguration::setOpenFirst(const bool f)
{
    m_session->setOpenFirst(f);
}
inline void PluginConfiguration::setUseWildcardSearch(const bool f)
{
    m_session->setUseWildcardSearch(f);
}
inline void PluginConfiguration::setWhatToMonitor(const SessionPluginConfiguration::EnumMonitorTargets::type v)
{
    m_session->setMonitorTargets(int(v));
    Q_EMIT(dirWatchSettingsChanged());
}
inline void PluginConfiguration::setSanitizeCompletions(const bool f)
{
    m_session->setSanitizeCompletions(f);
}
inline void PluginConfiguration::setHighlightCompletions(const bool f)
{
    m_session->setHighlightCompletions(f);
}
inline void PluginConfiguration::setAutoCompletions(const bool f)
{
    m_session->setAutoCompletions(f);
}
inline void PluginConfiguration::setIncludeMacros(const bool f)
{
    m_session->setIncludeMacros(f);
}
inline void PluginConfiguration::setUsePrefixColumn(const bool f)
{
    m_session->setUsePrefixColumn(f);
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
