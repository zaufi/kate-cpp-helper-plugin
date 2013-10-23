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

// Standard includes
#include <boost/filesystem/operations.hpp>
#include <KDE/KDebug>
#include <KDE/KGlobal>
#include <KDE/KSharedConfig>
#include <KDE/KSharedConfigPtr>
#include <KDE/KStandardDirs>
#include <QtCore/QFileInfo>

namespace kate { namespace {
/// \attention Make sure this path replaced everywhre in case of changes
/// \todo Make a constant w/ single declaration place for this path
const QString DATABASES_DIR = "plugins/katecpphelperplugin/indexed-collections/";
const QString DB_MANIFEST_FILE = DATABASES_DIR + "manifest";
namespace meta {
const QString GROUP_NAME = "options";
namespace key {
const QString PATH = "db-path";
const QString NAME = "name";
const QString COMMENT = "comment";
const QString TARGETS = "targets";
}}}                                                         // namespace key, meta, anonymous namespace

#if 0
DatabaseManager::database_options::database_options(database_options&& other) noexcept
  : m_status(other.m_status)
{
    m_path.swap(other.m_path);
    m_targets.swap(other.m_targets);
    m_name.swap(other.m_name);
    m_comment.swap(other.m_comment);
}

auto DatabaseManager::database_options::operator=(database_options&& other) noexcept -> database_options&
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

/**
 * \brief Search for stored indexer databases
 *
 * \todo Handle the case when \c base_dir is not a directory actually
 */
DatabaseManager::DatabaseManager(const KUrl& base_dir, const QStringList& enabled_list)
  : m_enabled_list(enabled_list)
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
        if (it->path().filename() == "manifest")
        {
            kDebug(DEBUG_AREA) << "found manifest:" << it->path().c_str();
            /// \todo Handle errors
            auto state = try_load_database_meta(it->path());
            assert("Sanity check" && state.m_options);
            if (enabled_list.indexOf(state.m_options->name()))
            {
                const auto& db_path = state.m_options->path().toUtf8().constData();
                state.m_db.reset(new index::ro::database(db_path));
                state.m_status = database_state::status::enabled;
            }
            else
            {
                state.m_status = database_state::status::disabled;
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

KUrl DatabaseManager::get_default_base_dir()
{
    auto base_dir = KGlobal::dirs()->locateLocal("appdata", DATABASES_DIR, true);
    return base_dir;
}

auto DatabaseManager::try_load_database_meta(const boost::filesystem::path& manifest) -> database_state
{
    auto filename = QString{manifest.c_str()};
    auto db_meta = KSharedConfig::openConfig(filename, KConfig::SimpleConfig);

    database_state result;
    result.m_options.reset(new DatabaseOptions(db_meta));

    auto db_info = QFileInfo{result.m_options->path()};
    if (!db_info.exists() || !db_info.isDir())
    {
        throw exception::invalid_manifest(
            std::string{"DB path doesn't exists or not a dir: "} + result.m_options->path().toUtf8().constData()
          );
    }
    if (result.m_options->name().isEmpty())
        throw exception::invalid_manifest("Index name is not given");

    kDebug(DEBUG_AREA) << "Found DB: name:" << result.m_options->name() << ", path:" << result.m_options->path();
    return result;
}

void DatabaseManager::enable(const QString& name, const bool flag)
{
    for (auto& index : m_collections)
    {
        if (index.m_options->name() == name)
        {
            index.m_status = flag
              ? database_state::status::enabled
              : database_state::status::disabled
              ;
        }
    }
}

void DatabaseManager::enable(const int idx, const bool flag)
{
    assert("Sanity check" && 0 <= idx && std::size_t(idx) < m_collections.size());
    m_collections[idx].m_status = flag
      ? database_state::status::enabled
      : database_state::status::disabled
      ;
    /// \todo Emit a signal (to update config)
    /// \todo Add DB
}

QAbstractTableModel* DatabaseManager::getDatabasesTableModel()
{
    if (!m_indices_model)
        m_indices_model.reset(new IndicesTableModel{std::weak_ptr<DatabaseManager>{shared_from_this()}});
    return m_indices_model.get();
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
