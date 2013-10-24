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
#include <src/indices_table_model.h>
#include <src/indexing_targets_list_model.h>

// Standard includes
#include <boost/filesystem/operations.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <KDE/KDebug>
#include <KDE/KDirSelectDialog>
#include <KDE/KGlobal>
#include <KDE/KLocalizedString>
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
namespace meta {
const QString GROUP_NAME = "options";
namespace key {
const QString PATH = "db-path";
const QString NAME = "name";
const QString COMMENT = "comment";
const QString TARGETS = "targets";
}}}                                                         // namespace key, meta, anonymous namespace

#if 0
DatabaseManager::database_state::database_state(database_state&& other) noexcept
  : m_status(other.m_status)
{
    m_path.swap(other.m_path);
    m_targets.swap(other.m_targets);
    m_name.swap(other.m_name);
    m_comment.swap(other.m_comment);
}

auto DatabaseManager::database_state::operator=(database_state&& other) noexcept -> database_state&
{
    if (this != &other)
    {
        m_path.swap(other.m_path);
        m_targets.swap(other.m_targets);
        m_name.swap(other.m_name);
        m_comment.swap(other.m_comment);
    }
    return *this;
}
#endif

bool DatabaseManager::database_state::isOk() const
{
    return m_status == status::ok;
}

/**
 * \brief Search for stored indexer databases
 *
 * \todo Handle the case when \c base_dir is not a directory actually
 */
DatabaseManager::DatabaseManager(const KUrl& base_dir, const QStringList& enabled_list)
  : m_base_dir(base_dir)
  , m_enabled_list(enabled_list)
  , m_last_selected_index(-1)
  , m_last_selected_target(-1)
{
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
            if (enabled_list.indexOf(state.m_options->name()) != -1)
            {
                const auto& db_path = state.m_options->path().toUtf8().constData();
                state.m_db.reset(new index::ro::database(db_path));
                state.m_enabled = true;
                state.m_status = database_state::status::ok;
            }
            else
            {
                state.m_status = database_state::status::unknown;
                state.m_enabled = false;
            }
            m_collections.emplace_back(std::move(state));
        }
    }
}

DatabaseManager::~DatabaseManager()
{
    // Write possible modified manifests for all collections
    for (const auto& index : m_collections)
        index.m_options->writeConfig();
}

QAbstractTableModel* DatabaseManager::getDatabasesTableModel()
{
    if (!m_indices_model)
        m_indices_model.reset(new IndicesTableModel{std::weak_ptr<DatabaseManager>{shared_from_this()}});
    return m_indices_model.get();
}

QAbstractListModel* DatabaseManager::getTargetsListModel()
{
    if (!m_targets_model)
        m_targets_model.reset(new IndexingTargetsListModel{std::weak_ptr<DatabaseManager>{shared_from_this()}});
    return m_targets_model.get();
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
    const auto id = QString{to_string(UUID_GEN()).c_str()};
    auto db_path = KUrl{m_base_dir};
    db_path.addPath(id);
    kDebug(DEBUG_AREA) << "Add new collection to" << db_path;
    {
        auto db_dir = QDir{m_base_dir.toLocalFile()};
        db_dir.mkpath(id);
    }
    auto meta_path = db_path;
    meta_path.addPath(DB_MANIFEST_FILE);
    auto db_meta = KSharedConfig::openConfig(meta_path.toLocalFile(), KConfig::SimpleConfig);

    database_state result;
    result.m_options.reset(new DatabaseOptions{db_meta});
    result.m_options->setPath(db_path.toLocalFile());
    result.m_options->setName("New collection");

    assert("Sanity check" && m_indices_model);
    m_indices_model->appendNewRow(
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

void DatabaseManager::rebuildCurrentIndex()
{

}

void DatabaseManager::refreshCurrentTargets(const QModelIndex& index)
{
    assert(
        "Database index is out of range"
      && std::size_t(index.row()) < m_collections.size()
      );
    assert("Sanity check" && m_targets_model);
    m_targets_model->refresAll(
        [this, &index]()
        {
            m_last_selected_index = index.row();
        }
      );
}

void DatabaseManager::selectCurrentTarget(const QModelIndex& index)
{
    assert("Sanity check" && m_targets_model);
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
        assert("Sanity check" && m_targets_model);
        auto targets = m_collections[m_last_selected_index].m_options->targets();
        const auto& target_path = target.toLocalFile();
        // Do not allow duplicates!
        if (targets.indexOf(target_path) != -1)
        {
            /// \todo make some spam
            return;
        }
        const auto sz = targets.size();
        m_targets_model->appendNewRow(
            targets.size()
          , [this, &target_path, &targets]()
            {
                targets << target_path;
                m_collections[m_last_selected_index].m_options->setTargets(targets);
            }
          );
        assert("Sanity check" && targets.size() == sz + 1);
        m_last_selected_target = targets.size() - 1;
    }
}

void DatabaseManager::removeCurrentTarget()
{
    if (m_last_selected_index == -1)
        return;

    assert("Sanity check" && m_targets_model);
    assert(
        "Database index is out of range"
      && std::size_t(m_last_selected_index) < m_collections.size()
      );

    if (m_last_selected_target == -1)
        return;

    auto targets = m_collections[m_last_selected_index].m_options->targets();
    const auto sz = targets.size();
    assert("Index is out of range" && m_last_selected_target < sz);

    m_targets_model->removeRow(
        m_last_selected_target
      , [this, &targets](const int idx)
        {
            targets.removeAt(idx);
            m_collections[m_last_selected_index].m_options->setTargets(targets);
        }
      );

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
    auto db_meta = KSharedConfig::openConfig(filename, KConfig::SimpleConfig);

    database_state result;
    result.m_options.reset(new DatabaseOptions{db_meta});

    auto db_info = QFileInfo{result.m_options->path()};
    if (!db_info.exists() || !db_info.isDir())
    {
        throw exception::invalid_manifest(
            std::string{"DB path doesn't exists or not a dir: "} + result.m_options->path().toUtf8().constData()
          );
    }

    kDebug(DEBUG_AREA) << "Found DB: name:" << result.m_options->name() << ", path:" << result.m_options->path();
    return result;
}

void DatabaseManager::renameCollection(int idx, const QString& new_name)
{
    auto old_name = m_collections[idx].m_options->name();
    m_collections[idx].m_options->setName(new_name);
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

}                                                           // namespace kate
// kate: hl C++11/Qt4;
