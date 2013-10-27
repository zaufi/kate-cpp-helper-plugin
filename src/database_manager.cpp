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
#include <src/index/indexer.h>
#if 0
#include <src/scope_exit.hh>
#endif
#include <src/index/utils.h>

// Standard includes
#include <boost/filesystem/operations.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
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
const boost::uuids::string_generator UUID_PARSER;
boost::uuids::random_generator UUID_GEN;
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
}

DatabaseManager::DatabaseManager()
  : m_indices_model{*this}
  , m_targets_model{*this}
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
void DatabaseManager::reset(const QStringList& enabled_list, const KUrl& base_dir)
{
    assert(
        "DatabaseManager supposed to be default initialized"
      && m_base_dir.isEmpty()
      && m_collections.empty()
      && m_enabled_list.empty()
      );

    /// Initialize vital members
    m_base_dir = base_dir;
    m_enabled_list = enabled_list;

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
            /// \todo Handle errors
            auto state = tryLoadDatabaseMeta(it->path());
            assert("Sanity check" && state.m_options);
            auto name = state.m_options->name();
            bool is_enabled;
            if (m_enabled_list.indexOf(name) != -1)
            {
                const auto& db_path = state.m_options->path().toUtf8().constData();
                state.m_db.reset(new index::ro::database(db_path));
                is_enabled = true;
                state.m_status = database_state::status::ok;
            }
            else
            {
                state.m_status = database_state::status::unknown;
                is_enabled = false;
            }
            state.m_enabled = is_enabled;
            m_collections.emplace_back(std::move(state));
            if (is_enabled)
                Q_EMIT(indexStatusChanged(name, true));
        }
    }
    /// \todo Get rid of not-found indices?
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

/// \todo Doesn't looks good/efficient...
bool DatabaseManager::isEnabled(const int idx) const
{
    assert("Index is out of range" && std::size_t(idx) < m_collections.size());
    const auto& name = m_collections[idx].m_options->name();
    const auto result = m_enabled_list.indexOf(name) != -1;
    return result;
}

void DatabaseManager::enable(const int idx, const bool flag)
{
    assert("Sanity check" && 0 <= idx && std::size_t(idx) < m_collections.size());
    m_collections[idx].m_enabled = flag;
    const auto& name = m_collections[idx].m_options->name();
    kDebug(DEBUG_AREA) << "Seting" << name << ": is_enabled=" << flag;
    if (flag)
        m_enabled_list << name;
    else
        m_enabled_list.removeOne(name);
    Q_EMIT(indexStatusChanged(name, flag));
    /// \todo Add or shutdown DB
}

void DatabaseManager::createNewIndex()
{
    const auto uuid = UUID_GEN();
    const auto id = QString{to_string(uuid).c_str()};
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
    result.loadMetaFrom(meta_path.toLocalFile());
    result.m_options->setPath(db_path.toLocalFile());
    result.m_options->setName("New collection");
    result.m_options->setUuid(id);
    result.m_options->writeConfig();

    m_indices_model.appendNewRow(
        m_collections.size()
      , [this, &result]()
        {
            m_collections.emplace_back(std::move(result));
        }
      );
}

void DatabaseManager::removeCurrentIndex()
{

}

void DatabaseManager::stopIndexer()
{

}

void DatabaseManager::rebuildCurrentIndex()
{
    // Check if any index has been selected, and no other reindexing in progress
    if (m_last_selected_index == -1)
    {
        kDebug(DEBUG_AREA) << "Nothing selected...";
        /// \todo Make some spam?
        return;
    }
    if (m_indexer)
    {
        kDebug(DEBUG_AREA) << "Reindexing already in progress...";
        assert("Sanity check" && m_indexing_in_progress != -1);
        /// \todo Make some spam?
        return;
    }

    auto& state = m_collections[m_last_selected_index];
    const auto& name = state.m_options->name();
    Q_EMIT(reindexingStarted(QString{"Starting to rebuild index: %1"}.arg(name)));

    // Make sure DB path + ".reindexing" suffix doesn't exits
    boost::system::error_code error;
    auto reindexing_db_path = boost::filesystem::path{
        state.m_options->path().toUtf8().constData()
      }.replace_extension("reindexing");
    boost::filesystem::remove_all(reindexing_db_path, error);
    if (error)
    {
        Q_EMIT(reindexingFinished(QString{"Index rebuilding failed: %1"}.arg(name)));
        /// \todo Make some spam?
        return;
    }

    // Make a new indexer and provide it w/ targets to scan
    auto db_id = index::make_dbid(state.m_id);
    m_indexer.reset(
        new index::indexer{db_id, reindexing_db_path.string()}
      );
    m_indexer->set_compiler_options(m_compiler_options.get());

    if (state.m_options->targets().empty())
    {
        m_indexer.reset();
        Q_EMIT(reindexingFinished(QString{"Index rebuilding failed: %1"}.arg(name)));
        KPassivePopup::message(
            i18n("Error")
          , i18n(
              "No index targets specified for <icode>%1</icode>"
            , name
            )
          , qobject_cast<QWidget*>(this)
          );
        /// \todo Make some spam
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
    m_indices_model.refreshRow(m_indexing_in_progress = m_last_selected_index);

    // Shutdown possible opened DB and change status
    state.m_db.reset();
    state.m_status = database_state::status::reindexing;

    // Go!
    m_indexer->start();
}

void DatabaseManager::rebuildFinished()
{
    assert("Sanity check" && m_indexing_in_progress != -1);
    auto& state = m_collections[m_indexing_in_progress];
    m_indexer.reset();                                      // CLose DBs well

    // Enable DB in a table view
    state.m_status = database_state::status::ok;
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

    // Notify that we've done...
    const auto& name = state.m_options->name();
    Q_EMIT(reindexingFinished(QString{"Index rebuilding has finished: %1"}.arg(name)));
}

void DatabaseManager::refreshCurrentTargets(const QModelIndex& index)
{
    assert(
        "Database index is out of range"
      && std::size_t(index.row()) < m_collections.size()
      );
    m_targets_model.refresAll(
        [this, &index]()
        {
            m_last_selected_index = index.row();
        }
      );
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
        assert("Sanity check" && targets.size() == sz + 1);
        m_last_selected_target = targets.size() - 1;
        state.m_options->writeConfig();
        // Make some SPAM
#if 0
        auto msg = DiagnosticMessagesModel::Record{
            clang::location{doc->url(), range.start().line() + 1, range.start().column() + 1}
          , "Completion point"
          , DiagnosticMessagesModel::Record::type::debug
          }
#endif
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
    kDebug(DEBUG_AREA) << "Trying to parse UUID:" << result.m_options->uuid();
    result.m_id = UUID_PARSER(result.m_options->uuid().toUtf8().constData());

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
    idx = m_enabled_list.indexOf(old_name);
    if (idx != -1)
    {
        m_enabled_list.removeAt(idx);
        m_enabled_list << new_name;
        // Notify config only if index really was enabled
        Q_EMIT(indexNameChanged(old_name, new_name));
    }
}

KUrl DatabaseManager::getDefaultBaseDir()
{
    auto base_dir = KGlobal::dirs()->locateLocal("appdata", DATABASES_DIR, true);
    return base_dir;
}

void DatabaseManager::reportCurrentFile(QString msg)
{
    auto report = DiagnosticMessagesModel::Record{
        QString{"  indexing %1 ..."}.arg(msg)
      , DiagnosticMessagesModel::Record::type::info
      };
    Q_EMIT(diagnosticMessage(report));
}

void DatabaseManager::reportIndexingError(clang::location loc, QString msg)
{
    auto report = DiagnosticMessagesModel::Record{
        std::move(loc)
      , std::move(msg)
      , DiagnosticMessagesModel::Record::type::error
      };
    Q_EMIT(diagnosticMessage(report));
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
