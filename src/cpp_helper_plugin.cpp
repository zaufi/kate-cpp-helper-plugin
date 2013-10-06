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
      , SLOT(buildPCHIfAbsent())
      );
    // Connect self to document deletion to do some cleanup
    connect(
        application()->documentManager()
      , SIGNAL(documentWillBeDeleted(KTextEditor::Document*))
      , this
      , SLOT(removeDocumentInfo(KTextEditor::Document*))
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
void CppHelperPlugin::readSessionConfig(KConfigBase* cfg, const QString& groupPrefix)
{
    kDebug(DEBUG_AREA) << "** PLUGIN **: Reading session config: " << groupPrefix;
    config().readSessionConfig(cfg, groupPrefix);
    buildPCHIfAbsent();
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
      , SIGNAL(diagnosticMessage(DiagnosticMessagesModel::Record))
      , view
      , SLOT(addDiagnosticMessage(DiagnosticMessagesModel::Record))
      );
    return view;
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
        kDebug(DEBUG_AREA) << "Going to monitor system dirs for changes...";
        for (const QString& path : config().systemDirs())
            updateDirWatcher(path);
    }
    if (config().what_to_monitor() & 1)
    {
        kDebug(DEBUG_AREA) << "Going to monitor session dirs for changes...";
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
    kDebug(DEBUG_AREA) << "Clang options had changed, invalidating translation units...";
    m_units.clear();
}

void CppHelperPlugin::updateCurrentView()
{
    KTextEditor::View* view = application()->activeMainWindow()->activeView();
    if (view)
        updateDocumentInfo(view->document());
}

void CppHelperPlugin::updateDocumentInfo(KTextEditor::Document* doc)
{
    kDebug(DEBUG_AREA) << "(re)scan document " << doc->url() << " for #includes...";

    assert("Valid document expected" && doc);
    KTextEditor::MovingInterface* mv_iface = qobject_cast<KTextEditor::MovingInterface*>(doc);
    if (!mv_iface)
    {
        kDebug(DEBUG_AREA) << "No moving iface!!!!!!!!!!!";
        return;
    }

    // Try to remove prev collected info
    {
        CppHelperPlugin::doc_info_type::iterator it = m_doc_info.find(doc);
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
    // Show busy mouse pointer
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

    const QString pch_file_name = filename.toLocalFile() + ".kate.pch";
    try
    {
        addDiagnosticMessage(
            DiagnosticMessagesModel::Record(
                QString("Rebuilding PCH file: %1").arg(pch_file_name)
              , DiagnosticMessagesModel::Record::type::info
              )
          );
        kDebug(DEBUG_AREA) << "Going to produce a PCH file" << pch_file_name
            << "from" << config().precompiledHeaderFile();
        TranslationUnit pch_unit(
            localIndex()
          , filename
          , config().formCompilerOptions()
          , TranslationUnit::defaultPCHParseOptions()
          );
        pch_unit.storeTo(pch_file_name);
        config().setPrecompiledFile(pch_file_name);         // Set PCH file only if everything is Ok
    }
    catch (const TranslationUnit::Exception::ParseFailure& e)
    {
        addDiagnosticMessage(
            DiagnosticMessagesModel::Record(
                QString("PCH file parse failure: %1").arg(e.what())
              , DiagnosticMessagesModel::Record::type::error
              )
          );
        /// \todo Add an option to disable code completion w/o PCH file?
    }
    catch (const TranslationUnit::Exception::SaveFailure& e)
    {
        addDiagnosticMessage(
            DiagnosticMessagesModel::Record(
                QString("Fail to store PCH file: %1").arg(e.what())
              , DiagnosticMessagesModel::Record::type::error
              )
          );
    }
    catch (const TranslationUnit::Exception& e)
    {
        addDiagnosticMessage(
            DiagnosticMessagesModel::Record(
                QString("PCH file failure: %1").arg(e.what())
              , DiagnosticMessagesModel::Record::type::error
              )
          );
    }
    QApplication::restoreOverrideCursor();                  // Restore mouse pointer to normal
}

/**
 * \attention Clang 3.1 has some problems (core dump) on loading a PCH file if latter
 * doesn't exists before call to \c TranslationUnit ctor. As I saw in \c gdb, it has
 * (seem) uninitialized counter for tokens in a \c Preprocessor class (due some kind
 * of delayed initialization or smth like that)... So it is why presence of a PCH
 * must be checked before...
 * \todo Investigate a bug in clang 3.1
 */
void CppHelperPlugin::buildPCHIfAbsent()
{
    if (config().precompiledHeaderFile().isEmpty())
    {
        addDiagnosticMessage(
            DiagnosticMessagesModel::Record(
                QString("No PCH file configured! Code completion maybe slooow!")
              , DiagnosticMessagesModel::Record::type::warning
              )
          );
        kDebug(DEBUG_AREA) << "No PCH file configured! Code completion maybe slooow!";
        return;
    }

    const QString pch_file_name = config().precompiledHeaderFile().toLocalFile() + ".kate.pch";
    QFileInfo pi(pch_file_name);
    if (!pi.exists())
    {
        makePCHFile(config().precompiledHeaderFile());
    }
    else
    {
        config().setPrecompiledFile(pch_file_name);         // Ok, file exists!
        addDiagnosticMessage(
            DiagnosticMessagesModel::Record(
                QString("Using PCH file: %1").arg(pch_file_name)
              , DiagnosticMessagesModel::Record::type::info
              )
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
    KTextEditor::HighlightInterface* iface = qobject_cast<KTextEditor::HighlightInterface*>(doc);
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

TranslationUnit::unsaved_files_list_type CppHelperPlugin::makeUnsavedFilesList(
    KTextEditor::Document* doc
  ) const
{
    // Form unsaved files list
    TranslationUnit::unsaved_files_list_type unsaved_files;
    //  1) append this document to the list of unsaved files
    const QString this_filename = doc->url().toLocalFile();
    unsaved_files.push_back(qMakePair(this_filename, doc->text()));
    /// \todo Collect other unsaved files
    return unsaved_files;
}

DocumentInfo& CppHelperPlugin::getDocumentInfo(KTextEditor::Document* doc)
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
    KTextEditor::Document* doc
  , clang::DCXIndex& index
  , std::unique_ptr<TranslationUnit> translation_units_map_type::mapped_type::* unit_offset
  , const unsigned parse_options
  , const bool use_pch
  )
{
    // Make sure an entry for document exists
    std::unique_ptr<TranslationUnit>& unit = m_units[doc].*unit_offset;
    // Form unsaved files list
    /// \todo It would be better to monitor if code has changed before
    /// \c updateUnsavedFiles() and \c reparse()
    auto unsaved_files = makeUnsavedFilesList(doc);
    // Check if translation unit created
    if (!unit)
    {
        addDiagnosticMessage(
            DiagnosticMessagesModel::Record(
                QString("Initial parsing %1").arg(doc->url().toLocalFile())
              , DiagnosticMessagesModel::Record::type::info
              )
          );
        // No! Need to create one...
        // Form command line parameters
        //  1) collect configured system and session dirs and make -I option series
        QStringList options = config().formCompilerOptions();
        //  2) append PCH options
        kDebug(DEBUG_AREA) << config().precompiledHeaderFile();
        kDebug(DEBUG_AREA) << config().pchFile();
        if (use_pch && !config().pchFile().isEmpty())
            options << /*"-Xclang" << */"-include-pch" << config().pchFile().toLocalFile();

        // Parse it!
        unit.reset(
            new TranslationUnit(index, doc->url(), options, parse_options, unsaved_files)
          );
    }
    else
    {
        unit->updateUnsavedFiles(unsaved_files);
    }
    unit->reparse();
    return *unit;
}

void CppHelperPlugin::addDiagnosticMessage(DiagnosticMessagesModel::Record record)
{
    Q_EMIT(diagnosticMessage(record));
}

//END CppHelperPlugin
}                                                           // namespace kate
// kate: hl C++11/Qt4
