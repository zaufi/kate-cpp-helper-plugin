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
#include <src/utils.h>

// Standard includes
#include <kate/application.h>
#include <kate/documentmanager.h>
#include <kate/mainwindow.h>
#include <KDE/KAboutData>
#include <KDE/KDebug>
#include <KDE/KPluginFactory>
#include <KDE/KPluginLoader>
#include <KDE/KTextEditor/Editor>
#include <KDE/KTextEditor/MovingInterface>
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
    QObject* app
  , const QList<QVariant>&
  )
  : Kate::Plugin(static_cast<Kate::Application*>(app), "kate_cpphelper_plugin")
  /// \todo Make parameters to \c clang_createIndex() configurable?
  , m_local_index(clang_createIndex(1, 1))
  , m_index(clang_createIndex(0, 1))
  , m_hidden_doc(nullptr)
{
    assert("clang index expected to be valid" && m_local_index);

    // Register few types inQt meta-types system, so later we can use them
    // in signal/slots...
    qRegisterMetaType<clang::location>("clang::location");
    qRegisterMetaType<clang::diagnostic_message>("clang::diagnostic_message");

    // Connect self to configuration updates
    connect(
        &m_config
      , SIGNAL(dirWatchSettingsChanged())
      , this
      , SLOT(updateDirWatcher())
      );
    connect(
        &m_config
      , SIGNAL(clangOptionsChanged())
      , this
      , SLOT(invalidateTranslationUnits())
      );
    connect(
        &m_config
      , SIGNAL(precompiledHeaderFileChanged())
      , this
      , SLOT(rebuildPCH())
      );
    // Connect self to document deletion to do some cleanup
    connect(
        application()->documentManager()
      , SIGNAL(documentWillBeDeleted(KTextEditor::Document*))
      , this
      , SLOT(removeDocumentInfo(KTextEditor::Document*))
      );
    // Subscribe config instance to database manager's events
    connect(
        &m_db_mgr
      , SIGNAL(indexStatusChanged(const QString&, bool))
      , &m_config
      , SLOT(setIndexState(const QString&, bool))
      );
    connect(
        &m_config
      , SIGNAL(clangOptionsChanged())
      , this
      , SLOT(propagateCompilerOptionsToIndexer())
      );
}

CppHelperPlugin::~CppHelperPlugin()
{
    kDebug(DEBUG_AREA) << "Unloading...";
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
/**
 * \attention This is a bad idea to initialize smth here that can be accessed
 * from other object, like view, cuz this method get called after view creation...
 */
void CppHelperPlugin::readSessionConfig(KConfigBase* cfg, const QString& groupPrefix)
{
    kDebug(DEBUG_AREA) << "** PLUGIN **: Reading session config: " << groupPrefix;
    config().readSessionConfig(cfg, groupPrefix);
    buildPCHIfAbsent(false);
    m_db_mgr.reset(config().enabledIndices());
    propagateCompilerOptionsToIndexer();
}

void CppHelperPlugin::writeSessionConfig(KConfigBase* cfg, const QString& groupPrefix)
{
    kDebug(DEBUG_AREA) << "** PLUGIN **: Writing session config: " << groupPrefix;
    config().writeSessionConfig(cfg, groupPrefix);
}
/**
 * Create a plugin view and subscribe it to \c diagnosticMessage signal
 * of \c this plugin instance, so diagnostic messages can be viewed in
 * a plugin's tool view.
 */
Kate::PluginView* CppHelperPlugin::createView(Kate::MainWindow* parent)
{
    auto* view = new CppHelperPluginView(parent, CppHelperPluginFactory::componentData(), this);
    // Connect view to diagnostic messages emit by this instance
    connect(
        this
      , SIGNAL(diagnosticMessage(clang::diagnostic_message))
      , view
      , SLOT(addDiagnosticMessage(clang::diagnostic_message))
      );
    return view;
}
//END Kate::Plugin interface implementation

void CppHelperPlugin::updateDirWatcher(const QString& path)
{
    assert("Sanity check" && m_dir_watcher);
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

    auto want_system =
        config().monitorTargets() == PluginConfiguration::MonitorTargets::both
     || config().monitorTargets() == PluginConfiguration::MonitorTargets::systemDirs
     ;
    auto want_ssession =
        config().monitorTargets() == PluginConfiguration::MonitorTargets::both
     || config().monitorTargets() == PluginConfiguration::MonitorTargets::systemDirs
     ;
    if (want_system)
    {
        kDebug(DEBUG_AREA) << "Going to monitor system dirs for changes...";
        for (const auto& path : config().systemDirs())
            updateDirWatcher(path);
    }
    if (want_ssession)
    {
        kDebug(DEBUG_AREA) << "Going to monitor session dirs for changes...";
        for (const auto& path : config().sessionDirs())
            updateDirWatcher(path);
    }

    m_dir_watcher->startScan(true);
}

void CppHelperPlugin::createdPath(const QString& path)
{
    // No reason to call update if it is just a dir was created...
    if (QFileInfo{path}.isFile() && m_last_updated != path)
    {
        kDebug(DEBUG_AREA) << "DirWatcher said created: " << path;
        updateCurrentView();
        m_last_updated = path;
    }
}

void CppHelperPlugin::deletedPath(const QString& path)
{
    if (m_last_updated != path)
    {
        kDebug(DEBUG_AREA) << "DirWatcher said deleted: " << path;
        updateCurrentView();
        m_last_updated = path;
    }
}

void CppHelperPlugin::invalidateTranslationUnits()
{
    addDiagnosticMessage(
        clang::diagnostic_message{
            QString{"Compiler options had changed, invalidating translation units..."}
          , clang::diagnostic_message::type::info
          }
      );
    m_units.clear();
}

void CppHelperPlugin::updateCurrentView()
{
    auto* view = application()->activeMainWindow()->activeView();
    if (view)
        updateDocumentInfo(view->document());
}

void CppHelperPlugin::updateDocumentInfo(KTextEditor::Document* doc)
{
    kDebug(DEBUG_AREA) << "(re)scan document " << doc->url() << " for #includes...";

    assert("Valid document expected" && doc);
    auto* mv_iface = qobject_cast<KTextEditor::MovingInterface*>(doc);
    if (!mv_iface)
    {
        kDebug(DEBUG_AREA) << "No moving iface!!!!!!!!!!!";
        return;
    }

    // Try to remove prev collected info
    {
        auto it = m_doc_info.find(doc);
        if (it != m_doc_info.end())
            m_doc_info.erase(it);
    }

    // Do we really need to scan this file?
    if (!isSuitableDocument(doc->mimeType(), doc->highlightingMode()))
    {
        kDebug(DEBUG_AREA) << "Document doesn't looks like C or C++: type ="
          << doc->mimeType() << ", hl =" << doc->highlightingMode();
        return;
    }

    auto di = std::unique_ptr<DocumentInfo>{new DocumentInfo(this)};

    // Search lines and filenames #include'd in this document
    for (auto i = 0, end_line = doc->lines(); i < end_line; i++)
    {
        const auto& line_str = doc->line(i);
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
    m_doc_info.insert(std::make_pair(doc, std::move(di)));
}

void CppHelperPlugin::removeDocumentInfo(KTextEditor::Document* doc)
{
    assert("Valid document expected" && doc);
    kDebug(DEBUG_AREA) << "going to remove document" << doc;
    // Try to remove previously collected info
    {
        auto it = m_doc_info.find(doc);
        if (it != end(m_doc_info))
            m_doc_info.erase(it);
    }
    // Remove translation unit for given document
    {
        auto it = m_units.find(doc);
        if (it != end(m_units))
            m_units.erase(it);
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
    invalidateTranslationUnits();

    // Show busy mouse pointer
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

    const auto pch_file_name = QString{filename.toLocalFile() + ".kate.pch"};
    try
    {
        addDiagnosticMessage(
            clang::diagnostic_message{
                QString{"Rebuilding PCH file: %1"}.arg(pch_file_name)
              , clang::diagnostic_message::type::info
              }
          );
        kDebug(DEBUG_AREA) << "Going to produce a PCH file" << pch_file_name
            << "from" << config().precompiledHeaderFile();
        auto pch_unit = TranslationUnit{
            localIndex()
          , filename
          , config().formCompilerOptions()
          , TranslationUnit::defaultPCHParseOptions()
          , clang::unsaved_files_list{}                     // NOTE Not used for PCH
          };
        pch_unit.storeTo(pch_file_name);
        config().setPrecompiledFile(pch_file_name);         // Set PCH file only if everything is Ok
    }
    catch (const TranslationUnit::Exception::ParseFailure& e)
    {
        addDiagnosticMessage(
            clang::diagnostic_message{
                QString{"PCH file parse failure: %1"}.arg(e.what())
              , clang::diagnostic_message::type::error
              }
          );
        /// \todo Add an option to disable code completion w/o PCH file?
    }
    catch (const TranslationUnit::Exception::SaveFailure& e)
    {
        addDiagnosticMessage(
            clang::diagnostic_message{
                QString{"Fail to store PCH file: %1"}.arg(e.what())
              , clang::diagnostic_message::type::error
              }
          );
    }
    catch (const TranslationUnit::Exception& e)
    {
        addDiagnosticMessage(
            clang::diagnostic_message(
                QString{"PCH file failure: %1"}.arg(e.what())
              , clang::diagnostic_message::type::error
              )
          );
    }
    QApplication::restoreOverrideCursor();                  // Restore mouse pointer to normal
}

void CppHelperPlugin::rebuildPCH()
{
    buildPCHIfAbsent(true);
}

/**
 * \attention Clang 3.1 has some problems (core dump) on loading a PCH file if latter
 * doesn't exists before call to \c TranslationUnit ctor. As I saw in \c gdb, it has
 * (seem) uninitialized counter for tokens in a \c Preprocessor class (due some kind
 * of delayed initialization or smth like that)... So it is why presence of a PCH
 * must be checked before...
 * \todo Investigate a bug in clang 3.1
 */
void CppHelperPlugin::buildPCHIfAbsent(const bool force_rebuild)
{
    if (config().precompiledHeaderFile().isEmpty())
    {
        addDiagnosticMessage(
            clang::diagnostic_message{
                QString{"No PCH file configured! Code completion maybe slooow!"}
              , clang::diagnostic_message::type::warning
              }
          );
        kDebug(DEBUG_AREA) << "No PCH file configured! Code completion maybe slooow!";
        return;
    }

    const auto pch_file_name = QString{config().precompiledHeaderFile().toLocalFile() + ".kate.pch"};
    auto pi = QFileInfo{pch_file_name};
    if (!pi.exists() || force_rebuild)
    {
        makePCHFile(config().precompiledHeaderFile());
    }
    else
    {
        config().setPrecompiledFile(pch_file_name);         // Ok, file exists!
        addDiagnosticMessage(
            clang::diagnostic_message{
                QString{"Using PCH file: %1"}.arg(pch_file_name)
              , clang::diagnostic_message::type::info
              }
          );
    }

    //
    kDebug(DEBUG_AREA) << "PCH header: " << config().precompiledHeaderFile();
    kDebug(DEBUG_AREA) << "PCH file: " << config().pchFile();
}

/**
 * Get a hidden document to be used to highlight code snippets.
 * Create a new instance if not initialized yet.
 *
 * \attention Caller (and other methods of this class) should never
 * use \c m_hidden_doc member directly!
 */
KTextEditor::Document* CppHelperPlugin::getHiddenDoc()
{
    if (!m_hidden_doc)
    {
        m_hidden_doc = application()->editor()->createDocument(this);
        // Document require a view to be able to highlight text
        m_hidden_doc->createView(nullptr);
    }
    return m_hidden_doc;
}

QList<KTextEditor::HighlightInterface::AttributeBlock> CppHelperPlugin::highlightSnippet(
    const QString& text
  , const QString& mode
  )
{
#if 0
    kDebug(DEBUG_AREA) << "Highligting text " << text << "using" << mode << "mode";
#endif
    QList<KTextEditor::HighlightInterface::AttributeBlock> result;
    auto* doc = getHiddenDoc();

    /// \todo Is there any reason to cache this pointer?
    /// (to avoid casting all the time to the same (?) value)
    /// Is it constant by the way?
    auto* iface = qobject_cast<KTextEditor::HighlightInterface*>(doc);
    if (iface)
    {
        doc->setHighlightingMode(mode);
        doc->setText(text);
        assert("Document expected to have at least 1 line" && doc->lines());
        /// \todo Why the document has \b default color scheme???
        result = iface->lineAttributes(0);
        doc->clear();
    }
    return result;
}

void CppHelperPlugin::updateUnsavedFiles()
{
    for (auto* doc : application()->editor()->documents())
    {
        const auto is_suitable_document = doc->isModified()
          && doc->url().isValid()
          && isSuitableDocument(doc->mimeType(), doc->highlightingMode())
          ;
        if (is_suitable_document)
            m_unsaved_files_cache.update(doc->url(), doc->text());
    }
    m_unsaved_files_cache.finalize_updating();
}

DocumentInfo& CppHelperPlugin::getDocumentInfo(KTextEditor::Document* const doc)
{
    // Find corresponding document info, insert if needed
    auto it = m_doc_info.find(doc);
    if (it == end(m_doc_info))
    {
        it = m_doc_info.insert(
            std::make_pair(doc, std::unique_ptr<DocumentInfo>(new DocumentInfo(this)))
          ).first;
    }
    return *it->second;
}

/**
 * \throw TranslationUnit::Exception
 */
TranslationUnit& CppHelperPlugin::getTranslationUnitByDocumentImpl(
    KTextEditor::Document* const doc
  , clang::DCXIndex& index
  , std::unique_ptr<TranslationUnit> translation_units_map_type::mapped_type::* unit_offset
  , const unsigned parse_options
  , const bool use_pch
  )
{
    // Make sure an entry for document exists
    auto& unit = m_units[doc].*unit_offset;
    // Form/update an internal unsaved files list
    updateUnsavedFiles();
    // Check if translation unit created
    if (!unit)
    {
        addDiagnosticMessage(
            clang::diagnostic_message{
                QString{"Parsing %1"}.arg(doc->url().toLocalFile())
              , clang::diagnostic_message::type::info
              }
          );
        // No! Need to create one...
        // Form command line parameters
        //  1) collect configured system and session dirs and make -I option series
        auto options = config().formCompilerOptions();
        //  2) append PCH options if any specified
        kDebug(DEBUG_AREA) << config().precompiledHeaderFile();
        kDebug(DEBUG_AREA) << config().pchFile();
        if (use_pch && !config().pchFile().isEmpty())
            options << /*"-Xclang" << */"-include-pch" << config().pchFile().toLocalFile();

        // Parse it!
        unit.reset(
            new TranslationUnit{index, doc->url(), options, parse_options, m_unsaved_files_cache}
          );
    }
    unit->reparse(m_unsaved_files_cache);
    return *unit;
}

void CppHelperPlugin::addDiagnosticMessage(const clang::diagnostic_message record)
{
    Q_EMIT(diagnosticMessage(record));
}

void CppHelperPlugin::propagateCompilerOptionsToIndexer()
{
    m_db_mgr.setCompilerOptions(config().formCompilerOptions());
}
//END CppHelperPlugin
}                                                           // namespace kate
// kate: hl C++11/Qt4
