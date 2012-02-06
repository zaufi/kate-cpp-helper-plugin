/**
 * \file
 *
 * \brief Class \c kate::include_helper_plugin_config_page (implementation)
 *
 * \date Mon Feb  6 06:04:17 MSK 2012 -- Initial design
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
#include <src/include_helper_plugin_config_page.h>
#include <src/include_helper_plugin.h>

// Standard includes
#include <KDebug>
#include <KDirSelectDialog>

namespace kate {
//BEGIN IncludeHelperPluginConfigPage
IncludeHelperPluginConfigPage::IncludeHelperPluginConfigPage(
    QWidget* parent
  , IncludeHelperPlugin* plugin
  )
  : Kate::PluginConfigPage(parent)
  , m_plugin(plugin)
{
    m_configuration_ui.setupUi(this);
    m_configuration_ui.addToGlobalButton->setIcon(KIcon("list-add"));
    m_configuration_ui.delFromGlobalButton->setIcon(KIcon("list-remove"));
    m_configuration_ui.addToSessionButton->setIcon(KIcon("list-add"));
    m_configuration_ui.delFromSessionButton->setIcon(KIcon("list-remove"));

    // Connect add/del buttons to actions
    connect(m_configuration_ui.addToGlobalButton, SIGNAL(clicked()), this, SLOT(addGlobalIncludeDir()));
    connect(m_configuration_ui.delFromGlobalButton, SIGNAL(clicked()), this, SLOT(delGlobalIncludeDir()));
    connect(m_configuration_ui.addToSessionButton, SIGNAL(clicked()), this, SLOT(addSessionIncludeDir()));
    connect(m_configuration_ui.delFromSessionButton, SIGNAL(clicked()), this, SLOT(delSessionIncludeDir()));

    // Populate configuration w/ dirs
    reset();
}

void IncludeHelperPluginConfigPage::apply()
{
    // Notify about configuration changes
    {
        QStringList dirs;
        for (int i = 0; i < m_configuration_ui.sessionDirsList->count(); ++i)
        {
            dirs.append(m_configuration_ui.sessionDirsList->item(i)->text());
        }
        m_plugin->setSessionDirs(dirs);
    }
    {
        QStringList dirs;
        for (int i = 0; i < m_configuration_ui.globalDirsList->count(); ++i)
        {
            dirs.append(m_configuration_ui.globalDirsList->item(i)->text());
        }
        m_plugin->setGlobalDirs(dirs);
    }
    m_plugin->setUseLtGt(m_configuration_ui.includeMarkersSwitch->checkState() == Qt::Checked);
}

void IncludeHelperPluginConfigPage::reset()
{
    kDebug() << "Reseting configuration";
    // Put dirs to the list
    m_configuration_ui.globalDirsList->addItems(m_plugin->globalDirs());
    m_configuration_ui.sessionDirsList->addItems(m_plugin->sessionDirs());
    m_configuration_ui.includeMarkersSwitch->setCheckState(
        m_plugin->useLtGt() ? Qt::Checked : Qt::Unchecked
      );
}

void IncludeHelperPluginConfigPage::addSessionIncludeDir()
{
    KUrl dir_uri = KDirSelectDialog::selectDirectory(KUrl(), true, this);
    if (dir_uri != KUrl())
    {
        const QString& dir_str = dir_uri.toLocalFile();
        if (!contains(dir_str, m_configuration_ui.sessionDirsList))
            new QListWidgetItem(dir_str, m_configuration_ui.sessionDirsList);
    }
}

void IncludeHelperPluginConfigPage::delSessionIncludeDir()
{
    delete m_configuration_ui.sessionDirsList->currentItem();
}

void IncludeHelperPluginConfigPage::addGlobalIncludeDir()
{
    KUrl dir_uri = KDirSelectDialog::selectDirectory(KUrl(), true, this);
    if (dir_uri != KUrl())
    {
        const QString& dir_str = dir_uri.toLocalFile();
        if (!contains(dir_str, m_configuration_ui.globalDirsList))
            new QListWidgetItem(dir_str, m_configuration_ui.globalDirsList);
    }
}

void IncludeHelperPluginConfigPage::delGlobalIncludeDir()
{
    delete m_configuration_ui.globalDirsList->currentItem();
}

bool IncludeHelperPluginConfigPage::contains(const QString& dir, const KListWidget* list)
{
    for (int i = 0; i < list->count(); ++i)
        if (list->item(i)->text() == dir)
            return true;
    return false;
}
//END IncludeHelperPluginConfigPage
}                                                           // namespace kate
