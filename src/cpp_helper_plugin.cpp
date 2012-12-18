/**
 * \file
 *
 * \brief Class \c kate::CppHelperPlugin (implementation)
 *
 * \date Sun Jan 29 09:15:53 MSK 2012 -- Initial design
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
#include <src/config.h>
#include <src/cpp_helper_plugin.h>
#include <src/cpp_helper_plugin_config_page.h>
#include <src/cpp_helper_plugin_view.h>
#include <src/document_info.h>
#include <src/translation_unit.h>
#include <src/utils.h>

// Standard includes
#include <kate/application.h>
#include <kate/mainwindow.h>
#include <KAboutData>
#include <KDebug>
#include <KPluginFactory>
#include <KPluginLoader>
#include <KTextEditor/Editor>
#include <KTextEditor/MovingInterface>
#include <QtCore/QFileInfo>

K_PLUGIN_FACTORY(CppHelperPluginFactory, registerPlugin<kate::CppHelperPlugin>();)
K_EXPORT_PLUGIN(
    CppHelperPluginFactory(
        KAboutData(
            "katecpphelperplugin"
          , "kate_cpphelper_plugin"
          , ki18n("C++ Helper Plugin")
          , PLUGIN_VERSION
          , ki18n("Helps to work w/ C/C++ code little more easy")
          , KAboutData::License_LGPL_V3
          )
      )
  )

namespace kate {
//BEGIN CppHelperPlugin
CppHelperPlugin::CppHelperPlugin(
    QObject* application
  , const QList<QVariant>&
  )
  : Kate::Plugin(static_cast<Kate::Application*>(application), "kate_cpphelper_plugin")
  /// \todo Make parameters to \c clang_createIndex() configurable?
  , m_index(clang_createIndex(0, 0))
  , m_hidden_doc(this->application()->editor()->createDocument(this))
{
    assert("clang index expected to be valid" && m_index);
    // Document require a view to be able to highlight text
    m_hidden_doc->createView(nullptr);
    // Connect self to configuration updates
    connect(
        &m_config
      , SIGNAL(dirWatchSettingsChanged())
      , this
      , SLOT(updateDirWatcher())
      );
    connect(
        &m_config
      , SIGNAL(precompiledHeaderFileChanged())
      , this
      , SLOT(buildPCHIfAbsent())
      );
}

CppHelperPlugin::~CppHelperPlugin()
{
    kDebug() << "Unloading...";
}

//BEGIN PluginConfigPageInterface interface implementation
uint CppHelperPlugin::configPages() const
{
    return 1;
}

QString CppHelperPlugin::configPageName(uint number) const
{
    Q_UNUSED(number)
    assert("This plugin have the only configuration page" && number == 0);
    return i18n("C++ Helper");
}

QString CppHelperPlugin::configPageFullName(uint number) const
{
    Q_UNUSED(number)
    assert("This plugin have the only configuration page" && number == 0);
    return i18n("C++ Helper Settings");
}

KIcon CppHelperPlugin::configPageIcon(uint number) const
{
    Q_UNUSED(number)
    assert("This plugin have the only configuration page" && number == 0);
    return KIcon ("text-x-c++src");
}

Kate::PluginConfigPage* CppHelperPlugin::configPage(uint number, QWidget* parent, const char*)
{
    assert("This plugin have the only configuration page" && number == 0);
    if (number != 0)
        return 0;
    return new CppHelperPluginConfigPage(parent, this);
}
//END PluginConfigPageInterface interface implementation

//BEGIN Kate::Plugin interface implementation
void CppHelperPlugin::readSessionConfig(KConfigBase* cfg, const QString& groupPrefix)
{
    config().readSessionConfig(cfg, groupPrefix);
    buildPCHIfAbsent();
}
void CppHelperPlugin::writeSessionConfig(KConfigBase* cfg, const QString& groupPrefix)
{
    config().writeSessionConfig(cfg, groupPrefix);
}
Kate::PluginView* CppHelperPlugin::createView(Kate::MainWindow* parent)
{
    return new CppHelperPluginView(parent, CppHelperPluginFactory::componentData(), this);
}
//END Kate::Plugin interface implementation

void CppHelperPlugin::updateDirWatcher(const QString& path)
{
    m_dir_watcher->addDir(path, KDirWatch::WatchSubDirs | KDirWatch::WatchFiles);
    connect(
        m_dir_watcher.get()
      , SIGNAL(created(const QString&))
      , this
      , SLOT(createdPath(const QString&))
      );
    connect(
        m_dir_watcher.get()
      , SIGNAL(deleted(const QString&))
      , this
      , SLOT(deletedPath(const QString&))
      );
}

void CppHelperPlugin::updateDirWatcher()
{
    if (m_dir_watcher)
        m_dir_watcher->stopScan();
    m_dir_watcher.reset(new KDirWatch(0));
    m_dir_watcher->stopScan();

    if (config().what_to_monitor() & 2)
    {
        kDebug() << "Going to monitor system dirs for changes...";
        for (const QString& path : config().systemDirs())
            updateDirWatcher(path);
    }
    if (config().what_to_monitor() & 1)
    {
        kDebug() << "Going to monitor session dirs for changes...";
        for (const QString& path : config().sessionDirs())
            updateDirWatcher(path);
    }

    m_dir_watcher->startScan(true);
}

void CppHelperPlugin::createdPath(const QString& path)
{
    // No reason to call update if it is just a dir was created...
    if (QFileInfo(path).isFile() && m_last_updated != path)
    {
        kDebug() << "DirWatcher said created: " << path;
        updateCurrentView();
        m_last_updated = path;
    }
}

void CppHelperPlugin::deletedPath(const QString& path)
{
    if (m_last_updated != path)
    {
        kDebug() << "DirWatcher said deleted: " << path;
        updateCurrentView();
        m_last_updated = path;
    }
}

void CppHelperPlugin::updateCurrentView()
{
    KTextEditor::View* view = application()->activeMainWindow()->activeView();
    if (view)
        updateDocumentInfo(view->document());
}

void CppHelperPlugin::updateDocumentInfo(KTextEditor::Document* doc)
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
        CppHelperPlugin::doc_info_type::iterator it = managed_docs().find(doc);
        if (it != managed_docs().end())
            managed_docs().erase(it);
    }

    std::unique_ptr<DocumentInfo> di(new DocumentInfo(this));

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
    managed_docs().insert(std::make_pair(doc, std::move(di)));
}

/// \todo Move this method to view class. View may access managed documents via accessor method.
void CppHelperPlugin::textInserted(KTextEditor::Document* doc, const KTextEditor::Range& range)
{
    kDebug() << doc << " new text: " << doc->text(range);
    KTextEditor::MovingInterface* mv_iface = qobject_cast<KTextEditor::MovingInterface*>(doc);
    if (!mv_iface)
    {
        kDebug() << "No moving iface!!!!!!!!!!!";
        return;
    }
    // Find corresponding document info, insert if needed
    doc_info_type::iterator it = managed_docs().find(doc);
    if (it == managed_docs().end())
    {
        it = managed_docs().insert(
            std::make_pair(doc, std::unique_ptr<DocumentInfo>(new DocumentInfo(this)))
          ).first;
    }
    // Search lines and filenames #include'd in this range
    for (int i = range.start().line(); i < range.end().line() + 1; i++)
    {
        const QString& line_str = doc->line(i);
        kate::IncludeParseResult r = parseIncludeDirective(line_str, true);
        if (r.m_range.isValid())
        {
            r.m_range.setBothLines(i);
            if (!it->second->isRangeWithSameExists(r.m_range))
            {
                it->second->addRange(
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

/// Used by config page to open a PCH header
void CppHelperPlugin::openDocument(const KUrl& pch_header)
{
    application()->activeMainWindow()->openUrl(pch_header);
}

/**
 * \todo There is a big problem w/ reparse of PCH (i.e. making sure that PCH is fresh):
 * if translation unit would be made from saved AST file, reparse will fail on that file,
 * and store will reduce size (dropping smth important), so later completer will totally fail.
 *
 * \todo Need to accept a command line parameters (and include dirs), cuz clicking 'Rebuild'
 * in configuration page should take all current (possible not saved options) to compile it.
 */
void CppHelperPlugin::makePCHFile(const KUrl& filename)
{
    // Show busy mouse pointer
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

    const QString pch_file_name = filename.toLocalFile() + ".kate.pch";
    try
    {
        kDebug() << "Going to produce a PCH file" << pch_file_name
            << "from" << config().precompiledHeaderFile();
        TranslationUnit pch_unit(
            index()
          , filename
          , config().formCompilerOptions()
          , TranslationUnit::defaultPCHParseOptions()
          );
        pch_unit.storeTo(pch_file_name);
    }
    catch (const TranslationUnit::Exception::ParseFailure&)
    {
        kError() << "PCH file failure. Code completion will be damn slow!";
        /// \todo Add an option to disable code completion w/o PCH file?
    }
    catch (const TranslationUnit::Exception::SaveFailure&)
    {
        kError() << "Unable to store a PCH file";
    }
    catch (const TranslationUnit::Exception& e)
    {
        kError() << "Smth wrong w/ the PCH file:" << e.what();
    }
    QApplication::restoreOverrideCursor();                  // Restore mouse pointer to normal
}

/**
 * \attention Clang 3.1 has some problems (core dump) on loading a PCH file if latter
 * doesn't exists before call to \c TranslationUnit ctor. As I so in \c gdb, it has
 * (seem) uninitialized counter for tokens in a \c Preprocessor class (due some kind
 * of delayed initialization or smth like that)... So it is why presence of a PCH
 * must be checked before...
 * \todo Investigate a bug in clang 3.1
 */
void CppHelperPlugin::buildPCHIfAbsent()
{
    if (config().precompiledHeaderFile().isEmpty())
    {
        kDebug() << "No PCH file configured! Code completion will be slooow!";
        return;
    }

    const QString pch_file_name = config().precompiledHeaderFile().toLocalFile() + ".kate.pch";
    QFileInfo pi(pch_file_name);
    if (!pi.exists())
        makePCHFile(config().precompiledHeaderFile());

    //
    kDebug() << "PCH header: " << config().precompiledHeaderFile();
    kDebug() << "PCH file: " << config().pchFile();
}

QList<KTextEditor::HighlightInterface::AttributeBlock> CppHelperPlugin::highlightSnippet(
    const QString& text
  , const QString& mode
  )
{
#if 0
    kDebug() << "Highligting text " << text << "using" << mode << "mode";
#endif
    QList<KTextEditor::HighlightInterface::AttributeBlock> result;

    /// \todo Is there any reason to cache this pointer?
    /// (to avoid casting all the time to the same (?) value)
    /// Is it constant by the way?
    KTextEditor::HighlightInterface* iface = qobject_cast<KTextEditor::HighlightInterface*>(m_hidden_doc);
    if (iface)
    {
        m_hidden_doc->setHighlightingMode(mode);
        m_hidden_doc->setText(text);
        assert("Document expected to have at least 1 line" && m_hidden_doc->lines());
        result = iface->lineAttributes(0);
        m_hidden_doc->clear();
    }
    return result;
}

//END CppHelperPlugin
}                                                           // namespace kate
// kate: hl C++11/Qt4
