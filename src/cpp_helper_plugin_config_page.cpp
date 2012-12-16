/**
 * \file
 *
 * \brief Class \c kate::CppHelperPluginConfigPage (implementation)
 *
 * \date Mon Feb  6 06:04:17 MSK 2012 -- Initial design
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

// Project specific includes
#include <src/cpp_helper_plugin_config_page.h>
#include <src/cpp_helper_plugin.h>
#include <src/utils.h>

// Standard includes
#include <KDebug>
#include <KDirSelectDialog>
#include <KPassivePopup>
#include <KShellCompletion>
#include <KStandardDirs>
#include <KTabWidget>
#include <QtGui/QVBoxLayout>
#include <cstdlib>

namespace kate {
//BEGIN CppHelperPluginConfigPage
CppHelperPluginConfigPage::CppHelperPluginConfigPage(
    QWidget* parent
  , CppHelperPlugin* plugin
  )
  : Kate::PluginConfigPage(parent)
  , m_plugin(plugin)
  , m_pss_config(new Ui_PerSessionSettingsConfigWidget())
  , m_clang_config(new Ui_CLangOptionsWidget())
  , m_system_list(new Ui_PathListConfigWidget())
  , m_session_list(new Ui_PathListConfigWidget())
  , m_compiler_paths(new Ui_DetectCompilerPathsWidget())
  , m_favorite_sets(new Ui_SessionPathsSetsWidget())
  , m_compiler_proc(this)
{
    QLayout* layout = new QVBoxLayout(this);
    KTabWidget* tab = new KTabWidget(this);
    layout->addWidget(tab);
    layout->setMargin(0);

    // Global #include paths
    {
        QWidget* system_tab = new QWidget(tab);
        // Setup paths list widget
        QWidget* paths = new QWidget(system_tab);
        paths->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_system_list->setupUi(paths);
        // Connect add/del buttons to actions
        connect(m_system_list->addButton, SIGNAL(clicked()), this, SLOT(addGlobalIncludeDir()));
        connect(m_system_list->delButton, SIGNAL(clicked()), this, SLOT(delGlobalIncludeDir()));
        connect(m_system_list->moveUpButton, SIGNAL(clicked()), this, SLOT(moveGlobalDirUp()));
        connect(m_system_list->moveDownButton, SIGNAL(clicked()), this, SLOT(moveGlobalDirDown()));
        connect(m_system_list->clearButton, SIGNAL(clicked()), this, SLOT(clearGlobalDirs()));
        // Setup predefined compiler's paths widget
        QWidget* compilers = new QWidget(system_tab);
        compilers->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_compiler_paths->setupUi(compilers);
        {
            auto gcc_binary = findBinary("g++");
            if (gcc_binary.isEmpty())
                m_compiler_paths->gcc->setEnabled(false);
            else
                m_compiler_paths->gcc->setText(gcc_binary);
        }
        {
            auto clang_binary = findBinary("clang++");
            if (clang_binary.isEmpty())
                m_compiler_paths->clang->setEnabled(false);
            else
                m_compiler_paths->clang->setText(clang_binary);
        }
        // Connect add button to action
        connect(m_compiler_paths->addButton, SIGNAL(clicked()), this, SLOT(detectPredefinedCompilerPaths()));
        // Setup layout
        QVBoxLayout* layout = new QVBoxLayout(system_tab);
        layout->addWidget(paths, 1);
        layout->addWidget(compilers, 0);
        system_tab->setLayout(layout);
        tab->addTab(system_tab, i18n("System Paths List"));
    }

    // Session #include paths
    {
        QWidget* session_tab = new QWidget(tab);
        // Setup paths list widget
        QWidget* paths = new QWidget(session_tab);
        paths->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_session_list->setupUi(paths);
        // Connect add/del buttons to actions
        connect(m_session_list->addButton, SIGNAL(clicked()), this, SLOT(addSessionIncludeDir()));
        connect(m_session_list->delButton, SIGNAL(clicked()), this, SLOT(delSessionIncludeDir()));
        connect(m_session_list->moveUpButton, SIGNAL(clicked()), this, SLOT(moveSessionDirUp()));
        connect(m_session_list->moveDownButton, SIGNAL(clicked()), this, SLOT(moveSessionDirDown()));
        connect(m_session_list->clearButton, SIGNAL(clicked()), this, SLOT(clearSessionDirs()));
        // Setup predefined compiler's paths widget
        QWidget* favorites = new QWidget(session_tab);
        favorites->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_favorite_sets->setupUi(favorites);
        updateSets();
        connect(m_favorite_sets->addButton, SIGNAL(clicked()), this, SLOT(addSet()));
        connect(m_favorite_sets->storeButton, SIGNAL(clicked()), this, SLOT(storeSet()));
        connect(m_favorite_sets->guessButton, SIGNAL(clicked()), this, SLOT(guessSet()));
        // Setup layout
        QVBoxLayout* layout = new QVBoxLayout(session_tab);
        layout->addWidget(paths, 1);
        layout->addWidget(favorites, 0);
        session_tab->setLayout(layout);
        tab->addTab(session_tab, i18n("Session Paths List"));
    }

    // Clang settings
    {
        QWidget* clang_tab = new QWidget(tab);
        m_clang_config->setupUi(clang_tab);
        m_clang_config->pchHeader->setUrl(m_plugin->config().precompiledHeaderFile());
        m_clang_config->commandLineParams->setPlainText(m_plugin->config().clangParams());
        pchHeaderChanged(m_plugin->config().precompiledHeaderFile());
        tab->addTab(clang_tab, i18n("Clang Settings"));
        // Monitor changes to PCH file
        connect(
            m_clang_config->pchHeader
          , SIGNAL(textChanged(const QString&))
          , this
          , SLOT(pchHeaderChanged(const QString&))
          );
        connect(
            m_clang_config->pchHeader
            // ATTENTION Documentation is wrong about signal parameter type!
            // http://api.kde.org/4.x-api/kdelibs-apidocs/kio/html/classKUrlRequester.html
          , SIGNAL(urlSelected(const KUrl&))
          , this
          , SLOT(pchHeaderChanged(const KUrl&))
          );
        // Connect open and rebuild PCH file button
        connect(m_clang_config->openPchHeader, SIGNAL(clicked()), this, SLOT(openPCHHeaderFile()));
        connect(m_clang_config->rebuildPch, SIGNAL(clicked()), this, SLOT(rebuildPCH()));
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

    // Subscribe self to compiler process signals
    connect(
        &m_compiler_proc
      , SIGNAL(error(QProcess::ProcessError))
      , this
      , SLOT(error(QProcess::ProcessError))
      );
    connect(
        &m_compiler_proc
      , SIGNAL(finished(int, QProcess::ExitStatus))
      , this
      , SLOT(finished(int, QProcess::ExitStatus))
      );
    connect(&m_compiler_proc, SIGNAL(readyReadStandardError()), this, SLOT(readyReadStandardError()));
    connect(&m_compiler_proc, SIGNAL(readyReadStandardOutput()), this, SLOT(readyReadStandardOutput()));

    // Populate configuration w/ dirs
    reset();
}

void CppHelperPluginConfigPage::apply()
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
void CppHelperPluginConfigPage::reset()
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

void CppHelperPluginConfigPage::addSessionIncludeDir()
{
    addDirTo(
        KDirSelectDialog::selectDirectory(KUrl(), true, this)
      , m_session_list->pathsList
      );
}

void CppHelperPluginConfigPage::delSessionIncludeDir()
{
    delete m_session_list->pathsList->currentItem();
}

void CppHelperPluginConfigPage::moveSessionDirUp()
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

void CppHelperPluginConfigPage::moveSessionDirDown()
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

void CppHelperPluginConfigPage::clearSessionDirs()
{
    m_session_list->pathsList->clear();
}

/**
 * Append configured paths from selected \c #include \c set
 */
void CppHelperPluginConfigPage::addSet()
{
    auto it = m_include_sets.find(m_favorite_sets->setsList->currentText());
    if (it != end(m_include_sets))
    {
        KConfigGroup general(it->second, "General");
        auto dirs = general.readPathEntry("Dirs", QStringList());
        for (const auto& dir : dirs)
            addDirTo(dir, m_session_list->pathsList);
    }
}

void CppHelperPluginConfigPage::storeSet()
{
    auto set_name = m_favorite_sets->setsList->currentText();
    kDebug() << "Current set name:" << set_name;

    KSharedConfigPtr cfg;
    {
        auto it = m_include_sets.find(set_name);
        if (it == end(m_include_sets))
        {
            auto filename = QString(QUrl::toPercentEncoding(set_name));
            auto incset_file = KStandardDirs::locateLocal(
                "appdata"
            , QString("plugins/katecpphelperplugin/%1.incset").arg(filename)
            , true
            );
            kDebug() << "Going to make a new incset file for it:" << incset_file;
            cfg = KSharedConfig::openConfig(incset_file, KConfig::SimpleConfig);
        }
        else cfg = it->second;
    }

    QStringList dirs;
    for (int i = 0, last = m_session_list->pathsList->count(); i < last; ++i)
        dirs << m_session_list->pathsList->item(i)->text();
    kDebug() << "Collected current paths:" << dirs;

    // Write Name and Dirs entries to the config
    KConfigGroup general(cfg, "General");
    general.writeEntry("Name", set_name);
    general.writePathEntry("Dirs", dirs);
    cfg->sync();
    updateSets();
}

void CppHelperPluginConfigPage::guessSet()
{

}

void CppHelperPluginConfigPage::addGlobalIncludeDir()
{
    addDirTo(
        KDirSelectDialog::selectDirectory(KUrl(), true, this)
      , m_system_list->pathsList
      );
}

void CppHelperPluginConfigPage::delGlobalIncludeDir()
{
    delete m_system_list->pathsList->currentItem();
}

void CppHelperPluginConfigPage::moveGlobalDirUp()
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

void CppHelperPluginConfigPage::moveGlobalDirDown()
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

void CppHelperPluginConfigPage::clearGlobalDirs()
{
    m_system_list->pathsList->clear();
}

/**
 * \note Silly Qt can't deal w/ signals and slots w/ different signatures,
 * and even nasty cheat like \c QSignalMapper can't help here to open a document
 * by pressing a button and get a current value from an other line edit control...
 * So it is why I have this one-liner... DAMN Qt!
 */
void CppHelperPluginConfigPage::openPCHHeaderFile()
{
    const auto& pch_url = m_clang_config->pchHeader->url();
    const auto& pch_file = pch_url.toLocalFile();
    if (!pch_file.isEmpty() && isPresentAndReadable(pch_file))
        m_plugin->openDocument(pch_url);
    else
        KPassivePopup::message(
            i18n("Error")
          , i18n("<qt>PCH header file is not configured or readable.</qt>")
          , qobject_cast<QWidget*>(this)
          );
}

void CppHelperPluginConfigPage::rebuildPCH()
{
    const auto& pch_url = m_clang_config->pchHeader->url();
    const auto& pch_file = pch_url.toLocalFile();
    if (!pch_file.isEmpty() && isPresentAndReadable(pch_file))
        m_plugin->makePCHFile(pch_url);
    else
        KPassivePopup::message(
            i18n("Error")
          , i18n("<qt>PCH header file is not configured or readable.</qt>")
          , qobject_cast<QWidget*>(this)
          );
}

void CppHelperPluginConfigPage::pchHeaderChanged(const QString& filename)
{
    const bool is_valid_pch_file = isPresentAndReadable(filename);
    kDebug() << "Check if PCH header file present and readable: "
      << filename << ", result=" << is_valid_pch_file;
    m_clang_config->openPchHeader->setEnabled(is_valid_pch_file);
    m_clang_config->rebuildPch->setEnabled(is_valid_pch_file);
}

void CppHelperPluginConfigPage::pchHeaderChanged(const KUrl& filename)
{
    pchHeaderChanged(filename.toLocalFile());
}

/**
 * \todo Need to raise a timer to detect a hunged process and kill it after timeout.
 */
void CppHelperPluginConfigPage::detectPredefinedCompilerPaths()
{
    const auto binary = getCurrentCompiler();
    kDebug() << "Determine predefined compiler paths for" << binary;

    m_output.clear();
    m_error.clear();
    m_compiler_proc.clearProgram();
    m_compiler_proc << binary << "-v" << "-E" << "-x" << "c++" << "/dev/null";
    m_compiler_proc.setOutputChannelMode(KProcess::SeparateChannels);
    m_compiler_proc.start();

    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    m_compiler_paths->addButton->setDisabled(true);
}

void CppHelperPluginConfigPage::error(QProcess::ProcessError error)
{
    const auto binary = getCurrentCompiler();
    const char* status_str;
    switch (error)
    {
        case QProcess::FailedToStart: status_str = "Process failed to start"; break;
        case QProcess::Crashed:       status_str = "Process crashed"; break;
        case QProcess::Timedout:      status_str = "Timedout";        break;
        case QProcess::WriteError:    status_str = "Write error";     break;
        case QProcess::ReadError:     status_str = "Read error";      break;
        case QProcess::UnknownError:
        default:
            status_str = "Unknown error";
            break;
    }
    KPassivePopup::message(
        i18n("Error")
      , i18n("<qt>Failed to execute <tt>%1</tt>:<br />%2</qt>", binary, status_str)
      , qobject_cast<QWidget*>(this)
      );
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
    m_compiler_paths->addButton->setDisabled(false);
}

void CppHelperPluginConfigPage::finished(int exit_code, QProcess::ExitStatus exit_status)
{
    kDebug() << "Compiler STDOUT: " << m_output;
    kDebug() << "Compiler STDERR: " << m_error;
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
    m_compiler_paths->addButton->setDisabled(false);

    // Do nothign on failure
    if (exit_status != QProcess::NormalExit && exit_code != EXIT_SUCCESS)
    {
        KPassivePopup::message(
            i18n("Error")
          , i18n(
                "<qt>Unable to get predefined <tt>#include</tt> paths. Process exited with code %1</qt>"
              , exit_code
              )
          , qobject_cast<QWidget*>(this)
          );
        return;
    }
    // Split output by lines
    auto lines = m_error.split('\n');
    bool collect_paths = false;
    for (const auto& line : lines)
    {
        if (line == QLatin1String("#include <...> search starts here:"))
        {
            collect_paths = true;
            continue;
        }
        if (line == QLatin1String("End of search list."))
        {
            collect_paths = false;
            continue;
        }
        if (collect_paths)
        {
            addDirTo(line.trimmed(), m_system_list->pathsList);
        }
    }
}

void CppHelperPluginConfigPage::readyReadStandardOutput()
{
    m_compiler_proc.setReadChannel(QProcess::StandardOutput);
    m_output.append(m_compiler_proc.readAll());
}

void CppHelperPluginConfigPage::readyReadStandardError()
{
    m_compiler_proc.setReadChannel(QProcess::StandardError);
    m_error.append(m_compiler_proc.readAll());
}

bool CppHelperPluginConfigPage::contains(const QString& dir, const KListWidget* list)
{
    for (int i = 0; i < list->count(); ++i)
        if (list->item(i)->text() == dir)
            return true;
    return false;
}

void CppHelperPluginConfigPage::addDirTo(const KUrl& dir_uri, KListWidget* list)
{
    if (dir_uri.isValid() && !dir_uri.isEmpty())
    {
        auto dir_str = dir_uri.toLocalFile();               // get URI as local file/path
        while (dir_str.endsWith('/'))                       // remove possible trailing slashes
            dir_str.remove(dir_str.length() - 1, 1);
        if (!contains(dir_str, list))                       // append only if given path not in a list already
            new QListWidgetItem(dir_str, list);
    }
}

QString CppHelperPluginConfigPage::findBinary(const QString& binary) const
{
    assert("binary name expected to be non-empty" && !binary.isEmpty());

    auto path_env = std::getenv("PATH");
    QString result;
    if (path_env)
    {
        /// \todo Is there any portable way to get paths separator?
        auto paths = QString(path_env).split(':', QString::SkipEmptyParts);
        for (auto&& path : paths)
        {
            const auto full_path = path + '/' + binary;
            const auto fi = QFileInfo(full_path);
            if (fi.exists() && fi.isExecutable())
            {
                result = full_path;
                break;
            }
        }
    }
    return result;
}

QString CppHelperPluginConfigPage::getCurrentCompiler() const
{
    QString binary;
    if (m_compiler_paths->gcc->isChecked())
        binary = findBinary("g++");
    else if (m_compiler_paths->clang->isChecked())
        binary = findBinary("clang++");

    return binary;
}

/**
 * Find all \c *.inset files in \c ${APPDATA}/plugins/katecpphelperplugin.
 * Open found file (as ordinal KDE config), read a set \c Name and fill the
 * \c m_include_sets map w/ \e Name to \c KSharedConfigPtr entry.
 * After all found files forcessed, fill the combobox w/ found entries.
 */
void CppHelperPluginConfigPage::updateSets()
{
    // Remove everything collected before
    m_favorite_sets->setsList->clear();
    m_include_sets.clear();

    // Find *.incset files
    auto sets = KGlobal::dirs()->findAllResources(
        "appdata"
      , "plugins/katecpphelperplugin/*.incset"
      , KStandardDirs::NoSearchOptions
      );
    kDebug() << "sets:" << sets;

    // Form a map of set names to shared configs
    for (const auto& filename : sets)
    {
        KSharedConfigPtr incset = KSharedConfig::openConfig(filename, KConfig::SimpleConfig);
        KConfigGroup general(incset, "General");
        auto set_name = general.readEntry("Name", QString());
        auto dirs = general.readPathEntry("Dirs", QStringList());
        kDebug() << "set name: " << set_name;
        kDebug() << "dirs: " << dirs;
        m_include_sets[set_name] = incset;
    }

    // Fill the `sets` combobox w/ names
    for (const auto& p : m_include_sets)
        m_favorite_sets->setsList->addItem(p.first);
}

//END CppHelperPluginConfigPage
}                                                           // namespace kate
// kate: hl C++11/Qt4;
