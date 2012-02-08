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
    m_configuration_ui.moveGlobalUpButton->setIcon(KIcon("arrow-up"));
    m_configuration_ui.moveGlobalDownButton->setIcon(KIcon("arrow-down"));
    m_configuration_ui.addToSessionButton->setIcon(KIcon("list-add"));
    m_configuration_ui.delFromSessionButton->setIcon(KIcon("list-remove"));
    m_configuration_ui.moveSessionUpButton->setIcon(KIcon("arrow-up"));
    m_configuration_ui.moveSessionDownButton->setIcon(KIcon("arrow-down"));

    // Connect add/del buttons to actions
    connect(m_configuration_ui.addToGlobalButton, SIGNAL(clicked()), this, SLOT(addGlobalIncludeDir()));
    connect(m_configuration_ui.delFromGlobalButton, SIGNAL(clicked()), this, SLOT(delGlobalIncludeDir()));
    connect(m_configuration_ui.moveGlobalUpButton, SIGNAL(clicked()), this, SLOT(moveGlobalDirUp()));
    connect(m_configuration_ui.moveGlobalDownButton, SIGNAL(clicked()), this, SLOT(moveGlobalDirDown()));

    connect(m_configuration_ui.addToSessionButton, SIGNAL(clicked()), this, SLOT(addSessionIncludeDir()));
    connect(m_configuration_ui.delFromSessionButton, SIGNAL(clicked()), this, SLOT(delSessionIncludeDir()));
    connect(m_configuration_ui.moveSessionUpButton, SIGNAL(clicked()), this, SLOT(moveSessionDirUp()));
    connect(m_configuration_ui.moveSessionDownButton, SIGNAL(clicked()), this, SLOT(moveSessionDirDown()));

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
    m_configuration_ui.sessionDirsList->removeItemWidget(
        m_configuration_ui.sessionDirsList->currentItem()
      );
}

void IncludeHelperPluginConfigPage::moveSessionDirUp() {
    const int current = m_configuration_ui.sessionDirsList->currentRow();
    if (current) {
        m_configuration_ui.sessionDirsList->insertItem(
            current - 1
          , m_configuration_ui.sessionDirsList->takeItem(current)
          );
        m_configuration_ui.sessionDirsList->setCurrentRow(current - 1);
    }
}

void IncludeHelperPluginConfigPage::moveSessionDirDown() {
    const int current = m_configuration_ui.sessionDirsList->currentRow();
    if (current < m_configuration_ui.sessionDirsList->count() - 1) {
        m_configuration_ui.sessionDirsList->insertItem(
            current + 1
          , m_configuration_ui.sessionDirsList->takeItem(current)
          );
        m_configuration_ui.sessionDirsList->setCurrentRow(current + 1);
    }
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
    m_configuration_ui.globalDirsList->removeItemWidget(
        m_configuration_ui.globalDirsList->currentItem()
      );
}

void IncludeHelperPluginConfigPage::moveGlobalDirUp() {
    const int current = m_configuration_ui.globalDirsList->currentRow();
    if (current) {
        m_configuration_ui.globalDirsList->insertItem(
            current - 1
          , m_configuration_ui.globalDirsList->takeItem(current)
          );
        m_configuration_ui.globalDirsList->setCurrentRow(current - 1);
    }
}

void IncludeHelperPluginConfigPage::moveGlobalDirDown() {
    const int current = m_configuration_ui.globalDirsList->currentRow();
    if (current < m_configuration_ui.globalDirsList->count() - 1) {
        m_configuration_ui.globalDirsList->insertItem(
            current + 1
          , m_configuration_ui.globalDirsList->takeItem(current)
          );
        m_configuration_ui.globalDirsList->setCurrentRow(current + 1);
    }
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
