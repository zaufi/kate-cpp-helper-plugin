/**
 * \file
 *
 * \brief Class \c kate::PluginConfiguration (implementation)
 *
 * \date Fri Nov 23 11:25:46 MSK 2012 -- Initial design
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

// Project specific includes
#include <src/plugin_configuration.h>

// Standard includes
#include <KDE/KConfigGroup>
#include <KDE/KDebug>
#include <KDE/KGlobal>
#include <KDE/KSharedConfig>
#include <KDE/KSharedConfigPtr>
#include <cassert>

namespace kate {
void PluginConfiguration::readConfig()
{
    // Read global config
    kDebug() << "Reading global config: " << KGlobal::config()->name();
    KConfigGroup gcg(KGlobal::config(), "IncludeHelper");
    QStringList dirs = gcg.readPathEntry("ConfiguredDirs", QStringList());
    kDebug() << "Got global configured include path list: " << dirs;
    m_system_dirs = dirs;
    //
    Q_EMIT(sessionDirsChanged());
    Q_EMIT(systemDirsChanged());
    Q_EMIT(dirWatchSettingsChanged());
}

void PluginConfiguration::readSessionConfig(KConfigBase* config, const QString& groupPrefix)
{
    kDebug() << "Reading session config: " << groupPrefix;
    // Read session config
    /// \todo Rename it!
    KConfigGroup scg(config, groupPrefix + ":include-helper");
    m_session_dirs = scg.readPathEntry("ConfiguredDirs", QStringList());
    m_pch_header= scg.readPathEntry("PCHFile", QString());
    m_clang_params = scg.readPathEntry("ClangCmdLineParams", QString());
    m_use_ltgt = scg.readEntry("UseLtGt", QVariant(false)).toBool();
    m_use_cwd = scg.readEntry("UseCwd", QVariant(false)).toBool();
    m_open_first = scg.readEntry("OpenFirstInclude", QVariant(false)).toBool();
    m_use_wildcard_search = scg.readEntry("UseWildcardSearch", QVariant(false)).toBool();
    m_monitor_flags = scg.readEntry("MonitorDirs", QVariant(0)).toInt();
    m_config_dirty = false;

    kDebug() << "Got per session configured include path list: " << m_session_dirs;

    readConfig();
}

void PluginConfiguration::writeSessionConfig(KConfigBase* config, const QString& groupPrefix)
{
    kDebug() << "Writing session config: " << groupPrefix;
    if (!m_config_dirty)
    {
        /// \todo Maybe I don't understand smth, but rally strange things r going on here:
        /// after plugin gets enabled, \c writeSessionConfig() will be called \b BEFORE
        /// any attempt to read configuration...
        /// The only thing came into my mind that it is attempt to initialize config w/ default
        /// values... but in that case everything stored before get lost!
        kDebug() << "Config isn't dirty!!!";
        readSessionConfig(config, groupPrefix);
        return;
    }

    // Write session config
    kDebug() << "Write per session configured include path list: " << m_session_dirs;
    KConfigGroup scg(config, groupPrefix + ":include-helper");
    scg.writePathEntry("ConfiguredDirs", m_session_dirs);
    scg.writeEntry("PCHFile", m_pch_header);
    scg.writeEntry("ClangCmdLineParams", m_clang_params);
    scg.writeEntry("UseLtGt", QVariant(m_use_ltgt));
    scg.writeEntry("UseCwd", QVariant(m_use_cwd));
    scg.writeEntry("OpenFirstInclude", QVariant(m_open_first));
    scg.writeEntry("UseWildcardSearch", QVariant(m_use_wildcard_search));
    scg.writeEntry("MonitorDirs", QVariant(m_monitor_flags));
    scg.sync();
    // Write global config
    kDebug() << "Write global configured include path list: " << m_system_dirs;
    KConfigGroup gcg(KGlobal::config(), "IncludeHelper");
    gcg.writePathEntry("ConfiguredDirs", m_system_dirs);
    gcg.sync();
    m_config_dirty = false;
}

void PluginConfiguration::setSessionDirs(QStringList& dirs)
{
    kDebug() << "Got session dirs: " << m_session_dirs;
    kDebug() << "... session dirs: " << dirs;
    if (m_session_dirs != dirs)
    {
        m_session_dirs.swap(dirs);
        m_config_dirty = true;
        Q_EMIT(sessionDirsChanged());
        Q_EMIT(dirWatchSettingsChanged());
        kDebug() << "** set config to `dirty' state!! **";
    }
}

void PluginConfiguration::setGlobalDirs(QStringList& dirs)
{
    kDebug() << "Got system dirs: " << m_system_dirs;
    kDebug() << "... system dirs: " << dirs;
    if (m_system_dirs != dirs)
    {
        m_system_dirs.swap(dirs);
        m_config_dirty = true;
        Q_EMIT(systemDirsChanged());
        Q_EMIT(dirWatchSettingsChanged());
        kDebug() << "** set config to `dirty' state!! **";
    }
}
void PluginConfiguration::setClangParams(const QString& params)
{
    if (m_clang_params != params)
    {
        m_clang_params = params;
        m_config_dirty = true;
        kDebug() << "** set config to `dirty' state!! **";
        Q_EMIT(precompiledHeaderFileChanged());
    }
}
void PluginConfiguration::setPrecompiledHeaderFile(const KUrl& filename)
{
    if (m_pch_header != filename)
    {
        m_pch_header = filename;
        m_config_dirty = true;
        kDebug() << "** set config to `dirty' state!! **";
        Q_EMIT(precompiledHeaderFileChanged());
    }
}
void PluginConfiguration::setUseLtGt(const bool state)
{
    m_use_ltgt = state;
    m_config_dirty = true;
    kDebug() << "** set config to `dirty' state!! **";
}

void PluginConfiguration::setUseCwd(const bool state)
{
    m_use_cwd = state;
    m_config_dirty = true;
    kDebug() << "** set config to `dirty' state!! **";
}
void PluginConfiguration::setOpenFirst(const bool state)
{
    m_open_first = state;
    m_config_dirty = true;
    kDebug() << "** set config to `dirty' state!! **";
}
void PluginConfiguration::setUseWildcardSearch(const bool state)
{
    m_use_wildcard_search = state;
    m_config_dirty = true;
    kDebug() << "** set config to `dirty' state!! **";
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

}                                                           // namespace kate
// kate: hl C++11/Qt4;
