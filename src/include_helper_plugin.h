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

#ifndef __SRC__INCLUDE_HELPER_PLUGIN_HH__
#   define __SRC__INCLUDE_HELPER_PLUGIN_HH__

// Project specific includes
#   include <src/ui_configuration.h>

// Standard includes
#   include <KAction>
#   include <KListWidget>
#   include <kate/application.h>
#   include <kate/documentmanager.h>
#   include <kate/mainwindow.h>
#   include <kate/plugin.h>
#   include <kate/pluginconfigpageinterface.h>
#   include <cassert>

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
    explicit IncludeHelperPluginGlobalConfigPage(QWidget* = 0, IncludeHelperPlugin* = 0);
    virtual ~IncludeHelperPluginGlobalConfigPage() {}

    /// \name PluginConfigPage interface implementation
    //@{
    void apply();
    void reset();
    void defaults() {}
    //@}

Q_SIGNALS:
    void sessionDirsUpdated(const QStringList&);
    void globalDirsUpdated(const QStringList&);

private Q_SLOTS:
    void addGlobalIncludeDir();                             ///< Add directory to the list
    void delGlobalIncludeDir();                             ///< Remove directory from the list
    void addSessionIncludeDir();                            ///< Add directory to the list
    void delSessionIncludeDir();                            ///< Remove directory from the list

private:
    bool contains(const QString&, const KListWidget*);      ///< Check if directories list contains given item

    IncludeHelperPlugin* m_plugin;                          ///< Parent plugin
    Ui_Configuration m_configuration_ui;                    ///< Configuration widget
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
    IncludeHelperPluginView(Kate::MainWindow*, const KComponentData&, IncludeHelperPlugin*);
    virtual ~IncludeHelperPluginView();

private Q_SLOTS:
    void openHeader();
    void viewChanged();

private:
    /// Get word under cursor
    QString currentWord();

    IncludeHelperPlugin* m_plugin;                          ///< Parent plugin
    KAction* m_open_header;                                 ///< <em>Open header</em> action
};

class ChooseFromListDialog : public KDialog
{
    Q_OBJECT

public:
    ChooseFromListDialog(QWidget*);
    static QStringList select(QWidget*, const QStringList&);

private:
    KListWidget* m_list;
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

    /// \name Accessors
    //@{
    const QStringList& sessionDirs() const
    {
        return m_session_dirs;
    }
    const QStringList& globalDirs() const
    {
        return m_global_dirs;
    }
    //@}

    /// \name PluginConfigPageInterface interface implementation
    //@{
    /// Get number of configuration pages for this plugin
    uint configPages() const
    {
        return 1;
    }
    /// Create a config page w/ given number and parent
    Kate::PluginConfigPage* configPage(uint = 0, QWidget* = 0, const char* = 0);
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

    /// \name Plugin interface implementation
    //@{
    void readSessionConfig(KConfigBase*, const QString&);
    void writeSessionConfig(KConfigBase*, const QString&);
    //@}

private Q_SLOTS:
    void updateSessionDirs(const QStringList&);
    void updateGlobalDirs(const QStringList&);

private:
    QStringList m_global_dirs;
    QStringList m_session_dirs;
};

#endif                                                      // __SRC__INCLUDE_HELPER_PLUGIN_HH__
