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
#include <src/include_helper_plugin.hh>

// Standard includes
#include <KAboutData>
#include <KDebug>
#include <KPluginFactory>
#include <KPluginLoader>

K_PLUGIN_FACTORY(IncludeHelperPluginFactory, registerPlugin<IncludeHelperPlugin>();)
K_EXPORT_PLUGIN(
    IncludeHelperPluginFactory(
        KAboutData(
            "kateincludehelperplugin"
          , "kate_includehelper_plugin"
          , ki18n("Include Helper Plugin")
          , "0.1"
          , ki18n("Helps to work w/ C/C++ headers little more easy")
          )
      )
  )

IncludeHelperPlugin::IncludeHelperPlugin(
    QObject* application
  , const QList<QVariant>&
  )
  : Kate::Plugin(static_cast<Kate::Application*>(application), "kate_includehelper_plugin")
{
}

Kate::PluginView* IncludeHelperPlugin::createView(Kate::MainWindow* parent)
{
    return new IncludeHelperPluginView(parent, IncludeHelperPluginFactory::componentData());
}

void IncludeHelperPlugin::readConfig()
{
    kDebug() << __PRETTY_FUNCTION__;
}


IncludeHelperPluginGlobalConfigPage::IncludeHelperPluginGlobalConfigPage(
    QWidget* parent
  , IncludeHelperPlugin* plugin
  )
  : Kate::PluginConfigPage(parent)
  , m_plugin(plugin)
{

}

void IncludeHelperPluginGlobalConfigPage::apply()
{
    kDebug() << __PRETTY_FUNCTION__;
}

void IncludeHelperPluginGlobalConfigPage::reset()
{
    kDebug() << __PRETTY_FUNCTION__;
}

IncludeHelperPluginView::IncludeHelperPluginView(Kate::MainWindow* mw, const KComponentData& data)
  : Kate::PluginView(mw)
  , Kate::XMLGUIClient(data)
{

}

void IncludeHelperPluginView::readSessionConfig(KConfigBase* /*config*/, const QString& /*groupPrefix*/)
{
    kDebug() << __PRETTY_FUNCTION__;
}
void IncludeHelperPluginView::writeSessionConfig(KConfigBase* /*config*/, const QString& /*groupPrefix*/)
{
    kDebug() << __PRETTY_FUNCTION__;
}
