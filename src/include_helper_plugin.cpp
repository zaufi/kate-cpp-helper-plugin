/**
 * \file
 *
 * \brief Class \c IncludeHelperPlugin (implementation)
 *
 * \date Sun Jan 29 09:15:53 MSK 2012 -- Initial design
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
#include <src/config.h>
#include <src/include_helper_plugin.h>
#include <src/include_helper_plugin_config_page.h>
#include <src/include_helper_plugin_view.h>
#include <src/document_info.h>

// Standard includes
#include <kate/application.h>
#include <KAboutData>
#include <KDebug>
#include <KPluginFactory>
#include <KPluginLoader>

K_PLUGIN_FACTORY(IncludeHelperPluginFactory, registerPlugin<kate::IncludeHelperPlugin>();)
K_EXPORT_PLUGIN(
    IncludeHelperPluginFactory(
        KAboutData(
            "kateincludehelperplugin"
          , "kate_includehelper_plugin"
          , ki18n("Include Helper Plugin")
          , PLUGIN_VERSION
          , ki18n("Helps to work w/ C/C++ headers little more easy")
          , KAboutData::License_LGPL_V3
          )
      )
  )

namespace kate {
//BEGIN IncludeHelperPlugin
IncludeHelperPlugin::IncludeHelperPlugin(
    QObject* application
  , const QList<QVariant>&
  )
  : Kate::Plugin(static_cast<Kate::Application*>(application), "kate_includehelper_plugin")
  , m_use_ltgt(true)
  , m_config_dirty(false)
{
}

IncludeHelperPlugin::~IncludeHelperPlugin()
{
    kDebug() << "Unloading...";
    Q_FOREACH(doc_info_type::mapped_type info, m_doc_info)
    {
        delete info;
    }
}

Kate::PluginView* IncludeHelperPlugin::createView(Kate::MainWindow* parent)
{
    return new IncludeHelperPluginView(parent, IncludeHelperPluginFactory::componentData(), this);
}

Kate::PluginConfigPage* IncludeHelperPlugin::configPage(uint number, QWidget* parent, const char*)
{
    assert("This plugin have the only configuration page" && number == 0);
    if (number != 0)
        return 0;
    return new IncludeHelperPluginConfigPage(parent, this);
}

void IncludeHelperPlugin::readConfig()
{
    // Read global config
    kDebug() << "** PLUGIN **: Reading global config: " << KGlobal::config()->name();
    KConfigGroup gcg(KGlobal::config(), "IncludeHelper");
    QStringList dirs = gcg.readPathEntry("ConfiguredDirs", QStringList());
    kDebug() << "Got global configured include path list: " << dirs;
    m_system_dirs = dirs;
}

void IncludeHelperPlugin::readSessionConfig(KConfigBase* config, const QString& groupPrefix)
{
    kDebug() << "** PLUGIN **: Reading session config: " << groupPrefix;
    // Read session config
    /// \todo Rename it!
    KConfigGroup scg(config, groupPrefix + ":include-helper");
    QStringList session_dirs = scg.readPathEntry("ConfiguredDirs", QStringList());
    QVariant use_ltgt = scg.readEntry("UseLtGt", QVariant(false));
    QVariant use_cwd = scg.readEntry("UseCwd", QVariant(false));
    kDebug() << "Got per session configured include path list: " << session_dirs;
    // Assign configuration
    m_session_dirs = session_dirs;
    m_use_ltgt = use_ltgt.toBool();
    m_use_cwd = use_cwd.toBool();
    m_config_dirty = false;

    readConfig();
}

void IncludeHelperPlugin::writeSessionConfig(KConfigBase* config, const QString& groupPrefix)
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
    scg.writeEntry("UseLtGt", QVariant(m_use_ltgt));
    scg.writeEntry("UseCwd", QVariant(m_use_cwd));
    scg.sync();
    // Write global config
    kDebug() << "Write global configured include path list: " << m_system_dirs;
    KConfigGroup gcg(KGlobal::config(), "IncludeHelper");
    gcg.writePathEntry("ConfiguredDirs", m_system_dirs);
    gcg.sync();
    m_config_dirty = false;
}

void IncludeHelperPlugin::setSessionDirs(QStringList& dirs)
{
    kDebug() << "Got session dirs: " << m_session_dirs;
    kDebug() << "... session dirs: " << dirs;
    if (m_session_dirs != dirs)
    {
        m_session_dirs.swap(dirs);
        m_config_dirty = true;
        Q_EMIT(sessionDirsChanged());
        kDebug() << "** set config to `dirty' state!! **";
    }
}

void IncludeHelperPlugin::setGlobalDirs(QStringList& dirs)
{
    kDebug() << "Got system dirs: " << m_system_dirs;
    kDebug() << "... system dirs: " << dirs;
    if (m_system_dirs != dirs)
    {
        m_system_dirs.swap(dirs);
        m_config_dirty = true;
        Q_EMIT(systemDirsChanged());
        kDebug() << "** set config to `dirty' state!! **";
    }
}

void IncludeHelperPlugin::setUseLtGt(const bool state)
{
    m_use_ltgt = state;
    m_config_dirty = true;
    kDebug() << "** set config to `dirty' state!! **";
}

void IncludeHelperPlugin::setUseCwd(const bool state)
{
    m_use_cwd = state;
    m_config_dirty = true;
    kDebug() << "** set config to `dirty' state!! **";
}

//END IncludeHelperPlugin
}                                                           // namespace kate
