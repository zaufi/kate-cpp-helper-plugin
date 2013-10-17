/**
 * \file
 *
 * \brief Class \c kate::database_manager (implementation)
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

// Standard includes
#include <boost/filesystem/operations.hpp>
#include <KDE/KDebug>
#include <KDE/KGlobal>
#include <KDE/KSharedConfig>
#include <KDE/KSharedConfigPtr>
#include <KDE/KStandardDirs>

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

database_manager::database_options::database_options(database_options&& other) noexcept
  : m_status(other.m_status)
{
    m_path.swap(other.m_path);
    m_targets.swap(other.m_targets);
    m_name.swap(other.m_name);
    m_comment.swap(other.m_comment);
}

auto database_manager::database_options::operator=(database_options&& other) noexcept -> database_options&
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

/**
 * \brief Search for stored indexer databases
 *
 * \todo Handle the case when \c base_dir is not a directory actually
 */
database_manager::database_manager(const KUrl& base_dir, const QStringList& enabled_list)
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
            auto options = try_load_database_meta(it->path());
            
        }
    }
}

database_manager::~database_manager()
{
}

KUrl database_manager::get_default_base_dir()
{
    auto base_dir = KGlobal::dirs()->locateLocal("appdata", DATABASES_DIR, true);
    return base_dir;
}

auto database_manager::try_load_database_meta(const boost::filesystem::path& manifest) -> database_options
{
    auto filename = QString{manifest.c_str()};
    auto db_meta = KSharedConfig::openConfig(filename, KConfig::SimpleConfig);
    auto meta_group = KConfigGroup{db_meta, meta::GROUP_NAME};

    database_options result;
    result.m_path = meta_group.readPathEntry(meta::key::PATH, QString());
    /// \todo Check that path is a direectory?
    auto db_path = boost::filesystem::path{result.m_path.toLocalFile().toUtf8().constData()};
    if (!exists(db_path))
        throw exception::invalid_manifest(std::string{"DB path doesn't exists: "} + db_path.string());
    result.m_name = meta_group.readEntry(meta::key::NAME, QString());
    if (result.m_name.isEmpty())
        throw exception::invalid_manifest("Index name is not given");
    result.m_comment = meta_group.readEntry(meta::key::COMMENT, QString());
    result.m_targets = meta_group.readPathEntry(meta::key::TARGETS, QStringList());
    kDebug(DEBUG_AREA) << "Found DB: name:" << result.m_name << ", path:" << result.m_path;
    return result;
}

void database_manager::enable(const QStringList&)
{
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
