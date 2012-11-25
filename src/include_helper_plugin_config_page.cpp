/**
 * \file
 *
 * \brief Class \c kate::IncludeHelperPluginConfigPage (implementation)
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
#include <KTabWidget>
#include <KShellCompletion>

namespace kate {
//BEGIN IncludeHelperPluginConfigPage
IncludeHelperPluginConfigPage::IncludeHelperPluginConfigPage(
    QWidget* parent
  , IncludeHelperPlugin* plugin
  )
  : Kate::PluginConfigPage(parent)
  , m_plugin(plugin)
  , m_pss_config(new Ui_PerSessionSettingsConfigWidget())
  , m_clang_config(new Ui_CLangOptionsWidget())
  , m_system_list(new Ui_PathListConfigWidget())
  , m_session_list(new Ui_PathListConfigWidget())
{
    QLayout* layout = new QVBoxLayout(this);
    KTabWidget* tab = new KTabWidget(this);
    layout->addWidget(tab);
    layout->setMargin(0);

    // Global #include paths
    {
        QWidget* system_tab = new QWidget(tab);
        m_system_list->setupUi(system_tab);
        m_system_list->addButton->setIcon(KIcon("list-add"));
        m_system_list->delButton->setIcon(KIcon("list-remove"));
        m_system_list->moveUpButton->setIcon(KIcon("arrow-up"));
        m_system_list->moveDownButton->setIcon(KIcon("arrow-down"));
        tab->addTab(system_tab, i18n("System Paths List"));
        // Connect add/del buttons to actions
        connect(m_system_list->addButton, SIGNAL(clicked()), this, SLOT(addGlobalIncludeDir()));
        connect(m_system_list->delButton, SIGNAL(clicked()), this, SLOT(delGlobalIncludeDir()));
        connect(m_system_list->moveUpButton, SIGNAL(clicked()), this, SLOT(moveGlobalDirUp()));
        connect(m_system_list->moveDownButton, SIGNAL(clicked()), this, SLOT(moveGlobalDirDown()));
    }

    // Session #include paths
    {
        QWidget* session_tab = new QWidget(tab);
        m_session_list->setupUi(session_tab);
        m_session_list->addButton->setIcon(KIcon("list-add"));
        m_session_list->delButton->setIcon(KIcon("list-remove"));
        m_session_list->moveUpButton->setIcon(KIcon("arrow-up"));
        m_session_list->moveDownButton->setIcon(KIcon("arrow-down"));
        tab->addTab(session_tab, i18n("Session Paths List"));
        // Connect add/del buttons to actions
        connect(m_session_list->addButton, SIGNAL(clicked()), this, SLOT(addSessionIncludeDir()));
        connect(m_session_list->delButton, SIGNAL(clicked()), this, SLOT(delSessionIncludeDir()));
        connect(m_session_list->moveUpButton, SIGNAL(clicked()), this, SLOT(moveSessionDirUp()));
        connect(m_session_list->moveDownButton, SIGNAL(clicked()), this, SLOT(moveSessionDirDown()));
    }

    // Clang settings
    {
        QWidget* clang_tab = new QWidget(tab);
        m_clang_config->setupUi(clang_tab);
        m_clang_config->pchHeader->setUrl(m_plugin->config().precompiledHeaderFile());
        m_clang_config->commandLineParams->setPlainText(m_plugin->config().clangParams());
        tab->addTab(clang_tab, i18n("Clang Settings"));
        // Connect open PCH file button
        connect(m_clang_config->openPchHeader, SIGNAL(clicked()), this, SLOT(openPCHHeaderFile()));
        connect(m_clang_config->rebuildPch, SIGNAL(clicked()), m_plugin, SLOT(refreshPCH()));
    }

    // Other settings
    {
        QWidget* pss_tab = new QWidget(tab);
        m_pss_config->setupUi(pss_tab);
        int flags = m_plugin->config().what_to_monitor();
        m_pss_config->nothing->setChecked(flags == 0);
        m_pss_config->session->setChecked(flags == 1);
        m_pss_config->system->setChecked(flags == 2);
        m_pss_config->all->setChecked(flags == 3);
        tab->addTab(pss_tab, i18n("Other Settings"));
    }

    // Populate configuration w/ dirs
    reset();
}

void IncludeHelperPluginConfigPage::apply()
{
    kDebug() << "** CONFIG-PAGE **: Applying configuration";
    // Notify about configuration changes
    {
        QStringList dirs;
        for (int i = 0; i < m_session_list->pathsList->count(); ++i)
            dirs.append(m_session_list->pathsList->item(i)->text());
        m_plugin->config().setSessionDirs(dirs);
    }
    {
        QStringList dirs;
        for (int i = 0; i < m_system_list->pathsList->count(); ++i)
            dirs.append(m_system_list->pathsList->item(i)->text());
        m_plugin->config().setGlobalDirs(dirs);
    }
    m_plugin->config().setPrecompiledHeaderFile(m_clang_config->pchHeader->text());
    m_plugin->config().setClangParams(m_clang_config->commandLineParams->toPlainText());
    m_plugin->config().setUseLtGt(m_pss_config->includeMarkersSwitch->checkState() == Qt::Checked);
    m_plugin->config().setUseCwd(m_pss_config->useCurrentDirSwitch->checkState() == Qt::Checked);
    m_plugin->config().setOpenFirst(m_pss_config->openFirstHeader->checkState() == Qt::Checked);
    m_plugin->config().setUseWildcardSearch(m_pss_config->useWildcardSearch->checkState() == Qt::Checked);
    m_plugin->config().setWhatToMonitor(
        int(m_pss_config->nothing->isChecked()) * 0
      + int(m_pss_config->session->isChecked()) * 1
      + int(m_pss_config->system->isChecked()) * 2
      + int(m_pss_config->all->isChecked()) * 3
      );
}

/// \todo This method should do a reset configuration to default!
void IncludeHelperPluginConfigPage::reset()
{
    kDebug() << "** CONFIG-PAGE **: Reseting configuration";
    m_plugin->config().readConfig();
    // Put dirs to the list
    m_system_list->pathsList->addItems(m_plugin->config().systemDirs());
    m_session_list->pathsList->addItems(m_plugin->config().sessionDirs());
    m_pss_config->includeMarkersSwitch->setCheckState(
        m_plugin->config().useLtGt() ? Qt::Checked : Qt::Unchecked
      );
    m_pss_config->useCurrentDirSwitch->setCheckState(
        m_plugin->config().useCwd() ? Qt::Checked : Qt::Unchecked
      );
    m_pss_config->openFirstHeader->setCheckState(
        m_plugin->config().shouldOpenFirstInclude() ? Qt::Checked : Qt::Unchecked
      );
    m_pss_config->useWildcardSearch->setCheckState(
        m_plugin->config().useWildcardSearch() ? Qt::Checked : Qt::Unchecked
      );
}

void IncludeHelperPluginConfigPage::addSessionIncludeDir()
{
    KUrl dir_uri = KDirSelectDialog::selectDirectory(KUrl(), true, this);
    if (dir_uri != KUrl())
    {
        const QString& dir_str = dir_uri.toLocalFile();
        if (!contains(dir_str, m_session_list->pathsList))
            new QListWidgetItem(dir_str, m_session_list->pathsList);
    }
}

void IncludeHelperPluginConfigPage::delSessionIncludeDir()
{
    delete m_session_list->pathsList->currentItem();
}

void IncludeHelperPluginConfigPage::moveSessionDirUp()
{
    const int current = m_session_list->pathsList->currentRow();
    if (current)
    {
        m_session_list->pathsList->insertItem(
            current - 1
          , m_session_list->pathsList->takeItem(current)
          );
        m_session_list->pathsList->setCurrentRow(current - 1);
    }
}

void IncludeHelperPluginConfigPage::moveSessionDirDown()
{
    const int current = m_session_list->pathsList->currentRow();
    if (current < m_session_list->pathsList->count() - 1)
    {
        m_session_list->pathsList->insertItem(
            current + 1
          , m_session_list->pathsList->takeItem(current)
          );
        m_session_list->pathsList->setCurrentRow(current + 1);
    }
}

void IncludeHelperPluginConfigPage::addGlobalIncludeDir()
{
    KUrl dir_uri = KDirSelectDialog::selectDirectory(KUrl(), true, this);
    if (dir_uri != KUrl())
    {
        const QString& dir_str = dir_uri.toLocalFile();
        if (!contains(dir_str, m_system_list->pathsList))
            new QListWidgetItem(dir_str, m_system_list->pathsList);
    }
}

void IncludeHelperPluginConfigPage::delGlobalIncludeDir()
{
    delete m_system_list->pathsList->currentItem();
}

void IncludeHelperPluginConfigPage::moveGlobalDirUp()
{
    const int current = m_system_list->pathsList->currentRow();
    if (current)
    {
        m_system_list->pathsList->insertItem(
            current - 1
          , m_system_list->pathsList->takeItem(current)
          );
        m_system_list->pathsList->setCurrentRow(current - 1);
    }
}

void IncludeHelperPluginConfigPage::moveGlobalDirDown()
{
    const int current = m_system_list->pathsList->currentRow();
    if (current < m_system_list->pathsList->count() - 1)
    {
        m_system_list->pathsList->insertItem(
            current + 1
          , m_system_list->pathsList->takeItem(current)
          );
        m_system_list->pathsList->setCurrentRow(current + 1);
    }
}

/**
 * \note Silly Qt can't deal w/ signals and slots w/ different signatures,
 * and even nasty cheat like \c QSignalMapper can't help here to open a document
 * by pressing a button and get a current value from an other line edit control...
 * So it is why I have this one-liner... DAMN Qt!
 */
void IncludeHelperPluginConfigPage::openPCHHeaderFile()
{
    m_plugin->openDocument(m_clang_config->pchHeader->url());
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
// kate: hl C++11/Qt4;
