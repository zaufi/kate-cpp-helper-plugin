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
#include <src/include_helper_plugin.h>
#include <src/include_helper_plugin_config_page.h>
#include <src/include_helper_plugin_view.h>

// Standard includes
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
          , "0.1"
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
{
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

void IncludeHelperPlugin::readSessionConfig(KConfigBase* config, const QString& groupPrefix)
{
    // Read session config
    KConfigGroup scg(config, groupPrefix + ":include-helper");
    QStringList session_dirs = scg.readPathEntry("ConfiguredDirs", QStringList());
    kDebug() << "Got per session configured include path list: " << session_dirs;
    // Read global config
    KConfigGroup gcg(KGlobal::config(), "IncludeHelper");
    QStringList dirs = gcg.readPathEntry("ConfiguredDirs", QStringList());
    kDebug() << "Got global configured include path list: " << dirs;
    QVariant use_ltgt = gcg.readEntry("UseLtGt", QVariant(false));
    // Assign configuration
    m_session_dirs = session_dirs;
    m_global_dirs = dirs;
    m_use_ltgt = use_ltgt.toBool();
}

void IncludeHelperPlugin::writeSessionConfig(KConfigBase* config, const QString& groupPrefix)
{
    // Write session config
    kDebug() << "Write per session configured include path list: " << m_session_dirs;
    KConfigGroup scg(config, groupPrefix + ":include-helper");
    scg.writePathEntry("ConfiguredDirs", m_session_dirs);
    scg.sync();
    // Read global config
    kDebug() << "Write global configured include path list: " << m_global_dirs;
    KConfigGroup gcg(KGlobal::config(), "IncludeHelper");
    gcg.writePathEntry("ConfiguredDirs", m_global_dirs);
    gcg.writeEntry("UseLtGt", QVariant(m_use_ltgt));
    gcg.sync();
}

//END IncludeHelperPlugin
}                                                           // namespace kate
