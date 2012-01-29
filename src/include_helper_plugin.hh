/**
 * \file
 *
 * \brief Class \c IncludeHelperPlugin (interface)
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

#ifndef __SRC__INCLUDE_HELPER_PLUGIN_H__
# define __SRC__INCLUDE_HELPER_PLUGIN_H__

// Project specific includes

// Standard includes
#include <kate/application.h>
#include <kate/documentmanager.h>
#include <kate/mainwindow.h>
#include <kate/plugin.h>
#include <kate/pluginconfigpageinterface.h>
#include <cassert>

class IncludeHelperPlugin;                                  // forward declaration

/**
 * \brief [Type brief class description here]
 *
 * [More detailed description here]
 *
 */

class IncludeHelperPluginGlobalConfigPage : public Kate::PluginConfigPage
{
    Q_OBJECT

public:
    explicit IncludeHelperPluginGlobalConfigPage(QWidget* = nullptr, IncludeHelperPlugin* = nullptr);
    ~IncludeHelperPluginGlobalConfigPage() {}

    /// \name PluginConfigPage interface implementation
    //@{
    void apply();
    void reset();
    void defaults() {}
    //@}

private:
    IncludeHelperPlugin* m_plugin;                          ///< Parent plugin
};

/**
 * \brief [Type brief class description here]
 *
 * [More detailed description here]
 *
 */
class IncludeHelperPluginView : public Kate::PluginView, public Kate::XMLGUIClient
{
    Q_OBJECT

public:
    IncludeHelperPluginView(Kate::MainWindow*, const KComponentData&);
    virtual ~IncludeHelperPluginView() {}

    /// \name PluginView interface implementation
    //@{
    void readSessionConfig(KConfigBase*, const QString&);
    void writeSessionConfig(KConfigBase*, const QString&);
    //@}

private:
};

/**
 * \brief [Type brief class description here]
 *
 * [More detailed description here]
 *
 */
class IncludeHelperPlugin : public Kate::Plugin, public Kate::PluginConfigPageInterface
{
    Q_OBJECT
    Q_INTERFACES(Kate::PluginConfigPageInterface)

public:
    explicit IncludeHelperPlugin(QObject* parent = 0, const QList<QVariant>& = QList<QVariant>());
    virtual ~IncludeHelperPlugin() {}

    /// Create a new view of this plugin for the given main window
    Kate::PluginView *createView(Kate::MainWindow*);

    /// Load global plugin's config
    void readConfig();

    /// \name PluginConfigPageInterface interface implementation
    //@{
    /// Get number of configuration pages for this plugin
    uint configPages() const
    {
        return 1;
    }
    /// Create a config page w/ given number and parent
    Kate::PluginConfigPage* configPage(uint number = 0, QWidget* parent = nullptr, const char* = nullptr)
    {
        assert("This plugin have the only configuration page" && number == 0);
        if (number != 0)
        {
            return nullptr;
        }
        return new IncludeHelperPluginGlobalConfigPage(parent, this);
    }
    /// Get short name of a config page by number
    QString configPageName(uint number = 0) const
    {
        assert("This plugin have the only configuration page" && number == 0);
        return "Include Dirs";
    }
    QString configPageFullName(uint number = 0) const
    {
        assert("This plugin have the only configuration page" && number == 0);
        return "List of include directories";
    }
    KIcon configPageIcon(uint number = 0) const
    {
        assert("This plugin have the only configuration page" && number == 0);
        return KIcon("text-x-c++hdr");
    }
    //@}

private:
};

#endif                                                      // __SRC__INCLUDE_HELPER_PLUGIN_H__
