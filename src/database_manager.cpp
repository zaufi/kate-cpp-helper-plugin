/**
 * \file
 *
 * \brief Class \c kate::DatabaseManager (implementation)
 *
 * \date Sun Oct 13 08:47:24 MSK 2013 -- Initial design
 */
/*
 * Copyright (C) 2011-2013 Alex Turbov, all rights reserved.
 * This is free software. It is licensed for use, modification and
 * redistribution under the terms of the GNU General Public License,
 * version 3 or later <http://gnu.org/licenses/gpl.html>
 *
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
#include <src/database_manager.h>
#include <src/index/document.h>
#include <src/index/document_extras.h>
#include <src/index/indexer.h>
#include <src/index/utils.h>
#include <src/string_cast.h>

// Standard includes
#include <boost/archive/binary_iarchive.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/string.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <KDE/KDebug>
#include <KDE/KDirSelectDialog>
#include <KDE/KGlobal>
#include <KDE/KLocalizedString>
#include <KDE/KPassivePopup>
#include <KDE/KSharedConfig>
#include <KDE/KSharedConfigPtr>
#include <KDE/KStandardDirs>
#include <QtCore/QAbstractTableModel>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtGui/QStringListModel>

namespace kate { namespace {
/// \attention Make sure this path replaced everywhre in case of changes
/// \todo Make a constant w/ single declaration place for this path
const QString DATABASES_DIR = "plugins/katecpphelperplugin/indexed-collections/";
const char* const DB_MANIFEST_FILE = "manifest";
boost::uuids::random_generator UUID_GEN;
const QString ANONYMOUS = "<anonymous>";
/// \todo Make it configurable?
constexpr std::size_t VISUAL_NOTIFICATION_THRESHOLD = 100;

namespace meta {
const QString GROUP_NAME = "options";
namespace key {
const QString PATH = "db-path";
const QString NAME = "name";
const QString COMMENT = "comment";
const QString TARGETS = "targets";
}}}                                                         // namespace key, meta, anonymous namespace

DatabaseManager::database_state::~database_state()
{
    if (m_options)
        m_options->writeConfig();
}

bool DatabaseManager::database_state::isOk() const
{
    return m_status == status::ok;
}

void DatabaseManager::database_state::loadMetaFrom(const QString& filename)
{
    auto db_meta = KSharedConfig::openConfig(filename, KConfig::SimpleConfig);
    m_options.reset(new DatabaseOptions{db_meta});
    const auto& uuid_str = m_options->uuid();
    /// \note When creating a new index UUID is empty!
    if (!uuid_str.isEmpty())
        m_id = index::fromString(m_options->uuid());
}

DatabaseManager::DatabaseManager()
  : m_indices_model{*this}
  , m_targets_model{*this}
  , m_search_results_model{*this}
  , m_last_selected_index{-1}
  , m_last_selected_target{-1}
  , m_indexing_in_progress{-1}
{
}

DatabaseManager::~DatabaseManager()
{
    // Write possible modified manifests for all collections
    for (const auto& index : m_collections)
        index.m_options->writeConfig();
}

/**
 * \brief Search for stored indexer databases
 *
 * \todo Handle the case when \c base_dir is not a directory actually
 */
void DatabaseManager::reset(const std::set<boost::uuids::uuid>& enabled_list, const KUrl& base_dir)
{
    assert(
        "DatabaseManager supposed to be default initialized"
      && m_base_dir.isEmpty()
      && m_collections.empty()
      && m_enabled_list.empty()
      );

    /// Initialize vital members
    m_base_dir = base_dir;

    kDebug(DEBUG_AREA) << "Use indexer DB path:" << base_dir.toLocalFile();
    // NOTE Qt4 lacks a recursive directory iterator, so use
    // boost instead...
    using boost::filesystem::recursive_directory_iterator;
    for (
        auto it = recursive_directory_iterator(base_dir.toLocalFile().toUtf8().constData())
      , last = recursive_directory_iterator()
      ; it != last
      ; ++it
      )
    {
        if (it->path().filename() == DB_MANIFEST_FILE)
        {
            kDebug(DEBUG_AREA) << "found manifest:" << it->path().c_str();
            {
                auto report = clang::diagnostic_message{
                    i18nc("@info/plain", "found manifest: %1", it->path().c_str())
                  , clang::diagnostic_message::type::debug
                  };
                Q_EMIT(diagnosticMessage(report));
            }
            /// \todo Handle errors
            auto state = tryLoadDatabaseMeta(it->path());
            assert("Sanity check" && state.m_options);
            // Prevent DB index w/ same ID
            {
                auto prev_it = std::find_if(
                    begin(m_collections)
                  , end(m_collections)
                  , [&state](const collections_type::value_type& s)
                    {
                        return s.m_id == state.m_id;
                    }
                  );
                if (prev_it != end(m_collections))
                {
                    kError(DEBUG_AREA) << "ATTENTION: Non unique UUID found: "
                      << to_string(state.m_id).c_str() << '@'
                      << it->path().c_str()
                      ;
                    auto report = clang::diagnostic_message{
                        i18nc("@info/plain", "found manifest w/ non unique UUID: %1", it->path().c_str())
                      , clang::diagnostic_message::type::debug
                      };
                    Q_EMIT(diagnosticMessage(report));
                    continue;
                }
            }
            bool is_enabled = false;
            if (enabled_list.find(state.m_id) != end(enabled_list))
            {
                const auto& db_path = state.m_options->path().toUtf8().constData();
                try
                {
                    state.m_db.reset(new index::ro::database(db_path));
                    state.m_status = database_state::status::ok;
                    is_enabled = true;
                }
                catch (...)
                {
                    reportError(i18n("Load failure '%1'", state.m_options->name()));
                    state.m_status = database_state::status::invalid;
                }
            }
            else
            {
                state.m_status = database_state::status::unknown;
            }
            state.m_enabled = is_enabled;
            const auto db_id = state.m_id;
            m_collections.emplace_back(std::move(state));
            if (is_enabled)
            {
                m_search_db.add_index(m_collections.back().m_db.get());
                m_enabled_list.insert(db_id);
                Q_EMIT(indexStatusChanged(index::toString(db_id), true));
            }
            else
            {
                Q_EMIT(indexStatusChanged(index::toString(db_id), false));
            }
            assert("Sanity check" && m_search_db.used_indices() == m_enabled_list.size());
        }
    }
    /// \note Get rid of not-found indices (from config file)
    for (const auto& id : enabled_list)
        if (m_enabled_list.find(id) == end(m_enabled_list))
            Q_EMIT(indexStatusChanged(index::toString(id), false));
}

/// \todo Doesn't looks good/efficient...
bool DatabaseManager::isEnabled(const int idx) const
{
    assert("Index is out of range" && std::size_t(idx) < m_collections.size());
    return m_enabled_list.find(m_collections[idx].m_id) != end(m_enabled_list);
}

void DatabaseManager::enable(const QString& name, const bool flag)
{
    auto idx = 0;
    for (auto& index : m_collections)
    {
        if (index.m_options->name() == name)
        {
            enable(idx, flag);
            break;
        }
        idx++;
    }
}

void DatabaseManager::enable(const int idx, const bool flag)
{
    assert("Sanity check" && 0 <= idx && std::size_t(idx) < m_collections.size());
    auto& state = m_collections[idx];
    const auto& name = state.m_options->name();
    kDebug(DEBUG_AREA) << "Seting" << name << ": is_enabled=" << flag;
    if (flag)
    {
        // Try to open index first...
        try
        {
            state.m_db.reset(
                new index::ro::database(
                    state.m_options->path().toUtf8().constData()
                  )
              );
        }
        catch (...)
        {
            reportError("Enabling failed", idx, true);
            state.m_status = database_state::status::invalid;
            return;
        }
        m_search_db.add_index(state.m_db.get());
        m_enabled_list.insert(state.m_id);
        state.m_status = database_state::status::ok;
    }
    else
    {
        m_enabled_list.erase(state.m_id);
        m_search_db.remove_index(state.m_db.get());
        state.m_db.reset();
        state.m_status = database_state::status::unknown;
    }
    state.m_enabled = flag;
    Q_EMIT(indexStatusChanged(index::toString(state.m_id), flag));
    assert("Sanity check" && m_search_db.used_indices() == m_enabled_list.size());
}

void DatabaseManager::createNewIndex()
{
    const auto uuid = UUID_GEN();
    const auto id = index::toString(uuid);
    auto db_path = KUrl{m_base_dir};
    db_path.addPath(id);
    kDebug(DEBUG_AREA) << "Add new collection to" << db_path;
    {
        auto db_dir = QDir{m_base_dir.toLocalFile()};
        db_dir.mkpath(id);
    }
    auto meta_path = db_path;
    meta_path.addPath(DB_MANIFEST_FILE);

    database_state result;
    assert("Sanity check" && !result.m_enabled);
    result.loadMetaFrom(meta_path.toLocalFile());
    result.m_options->setPath(db_path.toLocalFile());
    result.m_options->setName("New collection");
    result.m_options->setUuid(id);
    result.m_options->writeConfig();
    result.m_id = uuid;

    m_indices_model.appendNewRow(
        m_collections.size()
      , [this, &result]()
        {
            m_collections.emplace_back(std::move(result));
        }
      );
}

void DatabaseManager::indexLocalsToggled(const bool is_checked)
{
    // Check if any index has been selected, and no other reindexing in progress
    if (m_last_selected_index == -1)
    {
        KPassivePopup::message(
            i18nc("@title:window", "Error")
          , i18nc("@info", "No index selected...")
            /// \todo WTF?! \c nullptr can't be used here!?
          , reinterpret_cast<QWidget*>(0)
          );
        return;
    }

    auto& state = m_collections[m_last_selected_index];
    state.m_options->setIndexLocals(is_checked);
    state.m_options->writeConfig();
}

void DatabaseManager::indexImplicitsToggled(const bool is_checked)
{
    // Check if any index has been selected, and no other reindexing in progress
    if (m_last_selected_index == -1)
    {
        KPassivePopup::message(
            i18nc("@title:window", "Error")
          , i18nc("@info", "No index selected...")
            /// \todo WTF?! \c nullptr can't be used here!?
          , reinterpret_cast<QWidget*>(0)
          );
        return;
    }

    auto& state = m_collections[m_last_selected_index];
    state.m_options->setSkipImplicitTemplateInstantiations(is_checked);
    state.m_options->writeConfig();
}

void DatabaseManager::removeCurrentIndex()
{
    // Check if any index has been selected, and no other reindexing in progress
    if (m_last_selected_index == -1)
    {
        KPassivePopup::message(
            i18nc("@title:window", "Error")
          , i18nc("@info", "No index selected...")
            /// \todo WTF?! \c nullptr can't be used here!?
          , reinterpret_cast<QWidget*>(0)
          );
        return;
    }
    if (m_indexing_in_progress == m_last_selected_index)
    {
        /// \note If indexing already in progress \em Reindex
        /// should be disabled already...
        kDebug(DEBUG_AREA) << "Reindexing in progress...Stop it!";
        assert("Sanity check" && m_indexer);
        return;
    }
    // Disable if needed...
    enable(m_last_selected_index, false);

    // Take the state out of collection
    auto state = std::move(m_collections[m_last_selected_index]);
    // Remove it (taking care about models)
    m_targets_model.refreshAll(
        [this]()
        {
            m_last_selected_target = -1;
        }
      );
    auto last_selected_index = m_last_selected_index;
    m_last_selected_index = -1;
    m_indices_model.removeRow(
        last_selected_index
      , [this](const int row)
        {
            m_collections.erase(begin(m_collections) + row);
        }
      );
    auto name = state.m_options->name();
    auto path = state.m_options->path();
    state.m_options.reset();                                // Flush any unsaved options

    // Remove all files
    boost::system::error_code error;
    auto db_path = boost::filesystem::path{path.toUtf8().constData()};
    boost::filesystem::remove_all(db_path, error);
    if (error)
    {
        /// \todo Make some spam?
        return;
    }
}

void DatabaseManager::rebuildCurrentIndex()
{
    // Check if any index has been selected, and no other reindexing in progress
    if (m_last_selected_index == -1)
    {
        KPassivePopup::message(
            i18nc("@title:window", "Error")
          , i18nc("@info", "No index selected...")
            /// \todo WTF?! \c nullptr can't be used here!?
          , reinterpret_cast<QWidget*>(0)
          );
        return;
    }
    if (m_indexer)
    {
        /// \note If indexing already in progress \em Reindex
        /// should be disabled already...
        kDebug(DEBUG_AREA) << "Reindexing already in progress...";
        assert("Sanity check" && m_indexing_in_progress != -1);
        return;
    }

    auto& state = m_collections[m_last_selected_index];
    const auto& name = state.m_options->name();
    Q_EMIT(reindexingStarted(i18nc("@info/plain", "Starting to rebuild index: %1", name)));

    // Make sure DB path + ".reindexing" suffix doesn't exits
    boost::system::error_code error;
    auto reindexing_db_path = boost::filesystem::path{
        state.m_options->path().toUtf8().constData()
      }.replace_extension("reindexing");
    boost::filesystem::remove_all(reindexing_db_path, error);
    if (error)
    {
        Q_EMIT(
            reindexingFinished(
                i18nc(
                    "@info/plain"
                  , "Index '%1' rebuilding failed: %2"
                  , name
                  , error.message().c_str()
                  )
              )
          );
        /// \todo Make better spam?
        return;
    }

    // Make a new indexer and provide it w/ targets to scan
    auto db_id = index::make_dbid(state.m_id);
    kDebug(DEBUG_AREA) << "Make short DB ID:" << index::toString(state.m_id) << " --> " << db_id;
    m_indexer.reset(
        new index::indexer{db_id, reindexing_db_path.string()}
      );
    auto indexing_options = index::indexer::default_indexing_options();
    if (state.m_options->indexLocals())
        indexing_options |= CXIndexOpt_IndexFunctionLocalSymbols;
    if (state.m_options->skipImplicitTemplateInstantiations())
        indexing_options |= CXIndexOpt_IndexImplicitTemplateInstantiations;
    m_indexer->set_indexing_options(indexing_options).set_compiler_options(m_compiler_options.get());

    if (state.m_options->targets().empty())
    {
        m_indexer.reset();
        auto msg = i18nc(
              "@info/plain"
            , "No index targets specified for <icode>%1</icode>"
            , name
            );
        Q_EMIT(reindexingFinished(msg));
        KPassivePopup::message(
            i18n("Error")
          , msg
            /// \todo WTF?! \c nullptr can't be used here!?
          , reinterpret_cast<QWidget*>(0)
          );
        return;
    }

    for (auto& tgt : state.m_options->targets())
        m_indexer->add_target(tgt);

    // Subscribe self for indexer events
    connect(
        m_indexer.get()
      , SIGNAL(finished())
      , this
      , SLOT(rebuildFinished())
      );
    connect(
        m_indexer.get()
      , SIGNAL(indexing_uri(QString))
      , this
      , SLOT(reportCurrentFile(QString))
      );
    connect(
        m_indexer.get()
      , SIGNAL(message(clang::diagnostic_message))
      , this
      , SLOT(reportIndexingError(clang::diagnostic_message))
      );
    m_indices_model.refreshRow(m_indexing_in_progress = m_last_selected_index);

    // Shutdown possible opened DB and change status
    if (state.m_enabled)
    {
        m_search_db.remove_index(state.m_db.get());
        m_enabled_list.erase(state.m_id);                   // Remove temporarily from enabled list
    }
    assert("Sanity check" && m_search_db.used_indices() == m_enabled_list.size());
    state.m_db.reset();
    state.m_status = database_state::status::reindexing;

    // Go!
    m_indexer->start();
}

void DatabaseManager::stopIndexer()
{
    if (m_indexing_in_progress != -1)
    {
        assert("Sanity check" && m_indexer);
        m_indexer->stop();
    }
}

void DatabaseManager::rebuildFinished()
{
    assert("Sanity check" && m_indexing_in_progress != -1);
    auto& state = m_collections[m_indexing_in_progress];
    m_indexer.reset();                                      // CLose DBs well

    // Enable DB in a table view
    auto reindexed_db = m_indexing_in_progress;
    m_indexing_in_progress = -1;
    m_indices_model.refreshRow(reindexed_db);

    // Going to replace old index w/ a new one...
    const auto db_path = boost::filesystem::path{state.m_options->path().toUtf8().constData()};
    auto reindexing_db_path = db_path;
    reindexing_db_path.replace_extension("reindexing");

    boost::system::error_code error;
    // Keep database meta
    state.m_options.reset();                                // Flush meta
    auto meta_fileanme = db_path / DB_MANIFEST_FILE;
    boost::filesystem::copy(meta_fileanme, reindexing_db_path / DB_MANIFEST_FILE, error);
    if (error)
    {
        /// \todo Make some spam?
        return;
    }
    // Remove old index
    boost::filesystem::remove_all(db_path, error);
    if (error)
    {
        /// \todo Make some spam?
        return;
    }
    // Move new index instead
    boost::filesystem::rename(reindexing_db_path, db_path, error);
    if (error)
    {
        /// \todo Make some spam?
        return;
    }
    // Reload meta
    state.loadMetaFrom(meta_fileanme.string().c_str());

    const auto& name = state.m_options->name();
    // Reload index
    try
    {
        state.m_db.reset(new index::ro::database{db_path.string()});
        if (state.m_enabled)
        {
            m_search_db.add_index(state.m_db.get());
            m_enabled_list.insert(state.m_id);
        }
        assert("Sanity check" && m_search_db.used_indices() == m_enabled_list.size());
        state.m_status = database_state::status::ok;
    }
    catch (...)
    {
        reportError(i18n("Load failure '%1'", name));
        state.m_status = database_state::status::invalid;
        return;
    }

    // Notify that we've done...
    Q_EMIT(reindexingFinished(i18nc("@info/plain", "Index rebuilding has finished: %1", name)));
}

void DatabaseManager::refreshCurrentTargets(const QModelIndex& index)
{
    assert(
        "Database index is out of range"
      && std::size_t(index.row()) < m_collections.size()
      );
    m_targets_model.refreshAll(
        [this, &index]()
        {
            m_last_selected_index = index.row();
        }
      );
    const auto& options = *m_collections[m_last_selected_index].m_options;
    Q_EMIT(setIndexLocalsChecked(options.indexLocals()));
    Q_EMIT(setSkipImplicitsChecked(options.skipImplicitTemplateInstantiations()));
}

void DatabaseManager::selectCurrentTarget(const QModelIndex& index)
{
    assert(
        "Database index is out of range"
      && std::size_t(m_last_selected_index) < m_collections.size()
      );
    assert(
        "Index is out of range"
      && index.row() < m_collections[m_last_selected_index].m_options->targets().size()
      );
    m_last_selected_target = index.row();
}

void DatabaseManager::addNewTarget()
{
    if (m_last_selected_index == -1)
        return;

    assert(
        "Database index is out of range"
      && std::size_t(m_last_selected_index) < m_collections.size()
      );

    auto target = KDirSelectDialog::selectDirectory(
        KUrl()
      , true
      , nullptr
      , i18nc("@title:window", "Select index target")
      );
    if (target.isValid() && !target.isEmpty())
    {
        auto& state = m_collections[m_last_selected_index];
        auto targets = state.m_options->targets();
        const auto& target_path = target.toLocalFile();
        // Do not allow duplicates!
        if (targets.indexOf(target_path) != -1)
        {
            /// \todo make some spam
            return;
        }
        const auto sz = targets.size();
        m_targets_model.appendNewRow(
            targets.size()
          , [this, &target_path, &targets]()
            {
                targets << target_path;
                m_collections[m_last_selected_index].m_options->setTargets(targets);
            }
          );
        Q_UNUSED(sz);
        assert("Sanity check" && targets.size() == sz + 1);
        m_last_selected_target = targets.size() - 1;
        state.m_options->writeConfig();
    }
}

void DatabaseManager::removeCurrentTarget()
{
    if (m_last_selected_index == -1)
        return;

    assert(
        "Database index is out of range"
      && std::size_t(m_last_selected_index) < m_collections.size()
      );

    if (m_last_selected_target == -1)
        return;

    auto& state = m_collections[m_last_selected_index];
    auto targets = state.m_options->targets();
    const auto sz = targets.size();
    Q_UNUSED(sz);
    assert("Index is out of range" && m_last_selected_target < sz);

    kDebug(DEBUG_AREA) << "Remove target [" << state.m_options->name() << "]: " << targets[m_last_selected_target];
    m_targets_model.removeRow(
        m_last_selected_target
      , [this, &targets](const int idx)
        {
            targets.removeAt(idx);
            m_collections[m_last_selected_index].m_options->setTargets(targets);
        }
      );
    state.m_options->writeConfig();

    assert("Sanity check" && targets.size() == sz - 1);

    // Reset last selected target
    if (!targets.empty())
    {
        if (targets.size() <= m_last_selected_target)
            m_last_selected_target = targets.size() - 1;
    }
    else m_last_selected_target = -1;
}

auto DatabaseManager::tryLoadDatabaseMeta(const boost::filesystem::path& manifest) -> database_state
{
    auto filename = QString{manifest.c_str()};

    database_state result;
    result.loadMetaFrom(filename);

    auto db_info = QFileInfo{result.m_options->path()};
    if (!db_info.exists() || !db_info.isDir())
    {
        throw exception::invalid_manifest(
            std::string{"DB path doesn't exists or not a dir: %1"} + result.m_options->path().toUtf8().constData()
          );
    }

    kDebug(DEBUG_AREA) << "Found DB: name:" << result.m_options->name() << ", path:" << result.m_options->path();
    return result;
}

void DatabaseManager::renameCollection(int idx, const QString& new_name)
{
    auto old_name = m_collections[idx].m_options->name();
    m_collections[idx].m_options->setName(new_name);
    m_collections[idx].m_options->writeConfig();
}

KUrl DatabaseManager::getDefaultBaseDir()
{
    auto base_dir = KGlobal::dirs()->locateLocal("appdata", DATABASES_DIR, true);
    return base_dir;
}

void DatabaseManager::reportCurrentFile(QString msg)
{
    auto report = clang::diagnostic_message{
        i18nc("@info/plain", "  indexing %1 ...", msg)
      , clang::diagnostic_message::type::info
      };
    Q_EMIT(diagnosticMessage(report));
}

void DatabaseManager::reportIndexingError(clang::diagnostic_message msg)
{
    Q_EMIT(diagnosticMessage(msg));
}

/**
 * Forward search request to \c combined_index.
 * Latter will fill the model w/ results, so they will be displayed...
 */
void DatabaseManager::startSearch(QString query)
{
    assert("Sanity check" && m_search_db.used_indices() == m_enabled_list.size());
    if (m_enabled_list.empty())
    {
        KPassivePopup::message(
            i18nc("@title:window", "Error")
          , i18nc("@info:tooltip", "No indexed collections selected")
            /// \todo WTF?! \c nullptr can't be used here!?
          , reinterpret_cast<QWidget*>(0)
          );
        return;
    }
    try
    {
        auto search_results = m_search_db.search(query);
        auto& documents = search_results.first;
        // Make some SPAM: give user a hint about found/estimated results
        // if "too much" results found...
        if (VISUAL_NOTIFICATION_THRESHOLD < search_results.second)
        {
            if (documents.size() < search_results.second)
            {
                KPassivePopup::message(
                    i18nc("@title:window", "Search results")
                  , i18nc(
                        "@info:tooltip"
                      , "%1 results displayed of %2 estimated"
                      , documents.size()
                      , search_results.second
                      )
                    /// \todo WTF?! \c nullptr can't be used here!?
                  , reinterpret_cast<QWidget*>(0)
                  );
            }
            else
            {
                KPassivePopup::message(
                    i18nc("@title:window", "Search results")
                  , i18nc(
                        "@info:tooltip"
                      , "%1 results found"
                      , documents.size()
                      )
                    /// \todo WTF?! \c nullptr can't be used here!?
                  , reinterpret_cast<QWidget*>(0)
                  );
            }
        }
        //
        auto model_data = SearchResultsTableModel::search_results_list_type{};
        model_data.reserve(documents.size());
        // Transform Xapian::Documents into a model
        for (const auto& doc : documents)
        {
            // Get ID of an index produces this result
            auto tmp_str = doc.get_value(index::value_slot::DBID);
            if (tmp_str.empty())
            {
                kDebug(DEBUG_AREA) << "No DBID attached to a document";
                continue;
            }
            auto index_id = index::deserialize<index::dbid>(tmp_str);
            const auto& state = findIndexByID(index_id);
            const auto& index = *state.m_db;

            // Get kind
            tmp_str = doc.get_value(index::value_slot::KIND);
            assert("Sanity check" && !tmp_str.empty());

            // Form a search result item
            auto result = index::search_result{index::deserialize(tmp_str)};
            result.m_db_name = state.m_options->name();

            // Get source file ID
            tmp_str = doc.get_value(index::value_slot::FILE);
            if (tmp_str.empty())
            {
                kDebug(DEBUG_AREA) << "No source file ID attached to a document";
                continue;
            }
            auto file_id = static_cast<HeaderFilesCache::id_type>(Xapian::sortable_unserialise(tmp_str));
            // Resolve header ID into string
            result.m_file = index.headers_map()[file_id];

            // Get line/column
            result.m_line = static_cast<int>(
                Xapian::sortable_unserialise(doc.get_value(index::value_slot::LINE))
              );
            result.m_column = static_cast<int>(
                Xapian::sortable_unserialise(doc.get_value(index::value_slot::COLUMN))
              );

            // Get entity name
            {
                const auto& name = doc.get_value(index::value_slot::NAME);
                result.m_name = name.empty() ? ANONYMOUS : string_cast<QString>(name);
            }

            // Get entity type
            {
                const auto& type = doc.get_value(index::value_slot::TYPE);
                result.m_type = type.empty() ? QString{} : string_cast<QString>(type);
            }

            // Get parent scope
            {
                const auto& scope = doc.get_value(index::value_slot::SCOPE);
                if (!scope.empty())
                    result.m_scope = string_cast<QString>(scope);
            }

            // Get some other props
            {
                const auto& str = doc.get_value(index::value_slot::TEMPLATE);
                if (!str.empty())
                {
                    const auto value = index::deserialize<unsigned>(str);
                    result.m_template_kind = CXIdxEntityCXXTemplateKind(value);
                }
            }
            {
                const auto& str = doc.get_value(index::value_slot::FLAGS);
                if (!str.empty())
                    result.m_flags.m_flags_as_int =
                        index::deserialize<decltype(result.m_flags.m_flags_as_int)>(str);
            }
            {
                const auto& str = doc.get_value(index::value_slot::VALUE);
                if (!str.empty())
                    result.m_value = static_cast<long long>(Xapian::sortable_unserialise(str));
            }
            {
                const auto& str = doc.get_value(index::value_slot::SIZEOF);
                if (!str.empty())
                    result.m_sizeof = static_cast<std::size_t>(Xapian::sortable_unserialise(str));
            }
            {
                const auto& str = doc.get_value(index::value_slot::ALIGNOF);
                if (!str.empty())
                    result.m_alignof = static_cast<std::size_t>(Xapian::sortable_unserialise(str));
            }
            {
                const auto& str = doc.get_value(index::value_slot::ARITY);
                if (!str.empty())
                    result.m_arity = static_cast<int>(Xapian::sortable_unserialise(str));
            }
            {
                const auto& str = doc.get_value(index::value_slot::ACCESS);
                if (!str.empty())
                {
                    const auto value = index::deserialize<unsigned>(str);
                    result.m_access = CX_CXXAccessSpecifier(value);
                }
            }
            {
                const auto& str = doc.get_value(index::value_slot::BASES);
                if (!str.empty())
                {
                    std::stringstream ss{str, std::ios_base::in | std::ios_base::binary};
                    boost::archive::binary_iarchive ia{ss};
                    std::list<std::string> l;
                    ia >> l;
                    if (!l.empty())                         // Is there anything to transform?
                    {
                        auto bases = QStringList{};
                        for (const auto& base : l)
                            bases << string_cast<QString>(base);
                        result.m_bases = bases;
                    }
                }
            }

            //
            model_data.emplace_back(std::move(result));
        }
        // Update the model w/ obtained data
        m_search_results_model.updateSearchResults(std::move(model_data));
    }
    catch (...)
    {
        reportError("Search failure", -1, true);
    }
}

clang::location DatabaseManager::getSearchResultLocation(const int row) const
{
    const auto& sr = m_search_results_model.getSearchResult(row);
    return clang::location{sr.m_file, sr.m_line, sr.m_column};
}

auto DatabaseManager::findIndexByID(const index::dbid id) const -> const database_state&
{
    auto it = std::find_if(
        begin(m_collections)
      , end(m_collections)
      , [id](const database_state& state)
        {
            if (state.m_status == database_state::status::ok)
            {
                assert("Sanity check" && state.m_db);
                return state.m_db->id() == id;
            }
            return false;
        }
      );
    assert("Sanity check" && it != end(m_collections));
    return *it;
}

void DatabaseManager::reportError(const QString& prefix, const int index, const bool show_popup)
{
    assert("Sanity check" && (index == -1 || std::size_t(index) < m_collections.size()));
    try
    {
        throw;
    }
    catch (const std::exception& e)
    {
        auto msg = QString{};
        if (index != -1 && !prefix.isEmpty())
            msg = i18nc(
                "@info/plain"
              , "Index '%1': %2: %3"
              , m_collections[index].m_options->name()
              , prefix
              , e.what()
              );
        else if (index != -1 && prefix.isEmpty())
            msg = i18nc(
                "@info/plain"
              , "Index '%1': %2"
              , m_collections[index].m_options->name()
              , e.what()
              );
        else if (index == -1 && !prefix.isEmpty())
            msg = QString{"%1: %2"}.arg(prefix, e.what());
        else
            msg = e.what();
        //
        auto report = clang::diagnostic_message{
            msg
          , clang::diagnostic_message::type::error
          };
        Q_EMIT(diagnosticMessage(report));
        kDebug(DEBUG_AREA) << msg;

        // Show any popup also?
        if (show_popup)
            KPassivePopup::message(
                i18nc("@title:window", "Error")
              , msg
                /// \todo WTF?! \c nullptr can't be used here!?
              , reinterpret_cast<QWidget*>(0)
              );
    }
}

const index::search_result& DatabaseManager::getDetailsOf(const int index)
{
    return m_search_results_model.getSearchResult(index);
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
