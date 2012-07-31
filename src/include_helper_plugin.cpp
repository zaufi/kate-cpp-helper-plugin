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
#include <src/utils.h>

// Standard includes
#include <kate/application.h>
#include <kate/mainwindow.h>
#include <KAboutData>
#include <KDebug>
#include <KPluginFactory>
#include <KPluginLoader>
#include <KTextEditor/MovingInterface>
#include <QtCore/QFileInfo>

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
  , m_monitor_flags(0)
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
    updateDirWatcher();
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
    QVariant mon_dirs = scg.readEntry("MonitorDirs", QVariant(0));
    kDebug() << "Got per session configured include path list: " << session_dirs;
    // Assign configuration
    m_session_dirs = session_dirs;
    m_use_ltgt = use_ltgt.toBool();
    m_use_cwd = use_cwd.toBool();
    m_monitor_flags = mon_dirs.toInt();
    m_config_dirty = false;

    readConfig();
}

void IncludeHelperPlugin::writeSessionConfig(KConfigBase* config, const QString& groupPrefix)
{
    kDebug() << "** PLUGIN **: Writing session config: " << groupPrefix;
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
    scg.writeEntry("MonitorDirs", QVariant(m_monitor_flags));
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
        updateDirWatcher();
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
        updateDirWatcher();
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

void IncludeHelperPlugin::set_what_to_monitor(const int tgt)
{
    assert("Sanity check" && 0 <= tgt && tgt < 4);
    m_monitor_flags = tgt;
    m_config_dirty = true;
    updateDirWatcher();
}

void IncludeHelperPlugin::updateDirWatcher(const QString& path)
{
    m_dir_watcher->addDir(path, KDirWatch::WatchSubDirs | KDirWatch::WatchFiles);
    connect(
        m_dir_watcher.data()
      , SIGNAL(created(const QString&))
      , this
      , SLOT(createdPath(const QString&))
      );
    connect(
        m_dir_watcher.data()
      , SIGNAL(deleted(const QString&))
      , this
      , SLOT(deletedPath(const QString&))
      );
}

void IncludeHelperPlugin::updateDirWatcher()
{
    if (m_dir_watcher)
        m_dir_watcher->stopScan();
    m_dir_watcher = QSharedPointer<KDirWatch>(new KDirWatch(0));
    m_dir_watcher->stopScan();

    if (what_to_monitor() & 2)
    {
        kDebug() << "Going to monitor system dirs for changes...";
        Q_FOREACH(const QString& path, m_system_dirs)
            updateDirWatcher(path);
    }
    if (what_to_monitor() & 1)
    {
        kDebug() << "Going to monitor session dirs for changes...";
        Q_FOREACH(const QString& path, m_session_dirs)
            updateDirWatcher(path);
    }

    m_dir_watcher->startScan(true);
}

void IncludeHelperPlugin::createdPath(const QString& path)
{
    // No reason to call update if it is just a dir was created...
    if (QFileInfo(path).isFile() && m_last_updated != path)
    {
        kDebug() << "DirWatcher said created: " << path;
        updateCurrentView();
        m_last_updated = path;
    }
}

void IncludeHelperPlugin::deletedPath(const QString& path)
{
    if (m_last_updated != path)
    {
        kDebug() << "DirWatcher said deleted: " << path;
        updateCurrentView();
        m_last_updated = path;
    }
}

void IncludeHelperPlugin::updateCurrentView()
{
    KTextEditor::View* view = application()->activeMainWindow()->activeView();
    if (view)
    {
        updateDocumentInfo(view->document());
    }
}

void IncludeHelperPlugin::updateDocumentInfo(KTextEditor::Document* doc)
{
    assert("Valid document expected" && doc);
    kDebug() << "(re)scan document " << doc << " for #includes...";
    KTextEditor::MovingInterface* mv_iface = qobject_cast<KTextEditor::MovingInterface*>(doc);
    if (!mv_iface)
    {
        kDebug() << "No moving iface!!!!!!!!!!!";
        return;
    }

    // Try to remove prev collected info
    {
        IncludeHelperPlugin::doc_info_type::iterator it = managed_docs().find(doc);
        if (it != managed_docs().end())
        {
            delete *it;
            managed_docs().erase(it);
        }
    }

    DocumentInfo* di = new DocumentInfo(this);

    // Search lines and filenames #include'd in this document
    for (int i = 0, end_line = doc->lines(); i < end_line; i++)
    {
        const QString& line_str = doc->line(i);
        kate::IncludeParseResult r = parseIncludeDirective(line_str, false);
        if (r.m_range.isValid())
        {
            r.m_range.setBothLines(i);
            di->addRange(
                mv_iface->newMovingRange(
                    r.m_range
                  , KTextEditor::MovingRange::ExpandLeft | KTextEditor::MovingRange::ExpandRight
                  )
              );
        }
    }
    managed_docs().insert(doc, di);
}

void IncludeHelperPlugin::textInserted(KTextEditor::Document* doc, const KTextEditor::Range& range)
{
    kDebug() << doc << " new text: " << doc->text(range);
    KTextEditor::MovingInterface* mv_iface = qobject_cast<KTextEditor::MovingInterface*>(doc);
    if (!mv_iface)
    {
        kDebug() << "No moving iface!!!!!!!!!!!";
        return;
    }
    // Find corresponding document info, insert if needed
    IncludeHelperPlugin::doc_info_type::iterator it = managed_docs().find(doc);
    if (it == managed_docs().end())
    {
        it = managed_docs().insert(doc, new DocumentInfo(this));
    }
    // Search lines and filenames #include'd in this range
    for (int i = range.start().line(); i < range.end().line() + 1; i++)
    {
        const QString& line_str = doc->line(i);
        kate::IncludeParseResult r = parseIncludeDirective(line_str, true);
        if (r.m_range.isValid())
        {
            r.m_range.setBothLines(i);
            if (!(*it)->isRangeWithSameExists(r.m_range))
            {
                (*it)->addRange(
                    mv_iface->newMovingRange(
                        r.m_range
                      , KTextEditor::MovingRange::ExpandLeft | KTextEditor::MovingRange::ExpandRight
                      )
                  );
            } else kDebug() << "range already registered";
        }
        else kDebug() << "no valid #include found";
    }
}

//END IncludeHelperPlugin
}                                                           // namespace kate
