/**
 * \file
 *
 * \brief Class \c kate::index::database (implementation)
 *
 * \date Wed Oct  9 11:16:43 MSK 2013 -- Initial design
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
#include <src/index/database.h>

// Standard includes
#include <KDE/KDebug>

namespace kate { namespace index { namespace { namespace meta {
const std::string FILES_MAPPING = "HDRMAPCACHE";
const std::string DB_ID = "DBID";
}}                                                          // namespace meta, anonymous namespace

namespace rw {

database::database(const dbid db_id, const std::string& path) try
  : Xapian::WritableDatabase{path, Xapian::DB_CREATE_OR_OPEN}
  , details::database{db_id}
{
}
catch (const Xapian::DatabaseError& e)
{
    throw exception::database_failure{"Index database [" + path + "] failure: " + e.get_msg()};
}

database::~database()
{
    try
    {
        kDebug(DEBUG_AREA) << "Store DB meta [" << id() << "]...";
        set_metadata(meta::DB_ID, serialize(id()));
        set_metadata(meta::FILES_MAPPING, headers_map().storeToString());
    }
    catch (...)
    {
        kDebug(DEBUG_AREA) << "Fail to store DB meta";
    }
    commit();
}

void database::commit()
{
    try
    {
        kDebug(DEBUG_AREA) << "Commiting DB changes...";
        Xapian::WritableDatabase::commit();
    }
    catch (...)
    {
        /// \todo Handle errors (some of them are recoverable...)
        kDebug(DEBUG_AREA) << "Fail to commit";
    }
}

}                                                           // namespace rw

namespace ro {

/**
 * \brief Construct from existed database
 *
 * This constructor assume that database already exists (physically on a disk)
 */
database::database(const std::string& path) try
  : Xapian::Database{path}
  , details::database{}
{
    // Get internal DB ID
    auto db_id_str = static_cast<Database* const>(this)->get_metadata(meta::DB_ID);
    assert("Sanity check" && !db_id_str.empty());
    m_id = deserialize<decltype(m_id)>(db_id_str);
    // Load files mapping
    auto hdr_cache = static_cast<Database* const>(this)->get_metadata(meta::FILES_MAPPING);
    assert("Sanity check" && !hdr_cache.empty());
    m_files_cache.loadFromString(hdr_cache);
}
catch (const Xapian::DatabaseError& e)
{
    throw exception::database_failure{"Index database [" + path + "] failure: " + e.get_msg()};
}

database::~database()
{
}

}}}                                                         // namespace ro, index, kate
