/**
 * \file
 *
 * \brief Class \c kate::index::database (interface)
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

#pragma once

// Project specific includes
#include <src/index/details/database.h>

// Standard includes
#include <xapian.h>
#include <stdexcept>
#include <string>

namespace kate { namespace index {

/// Exceptions group for database classes
struct exception : public std::runtime_error
{
    struct database_failure;
    explicit exception(const std::string& str)
      : std::runtime_error(str)
    {}
};

struct exception::database_failure : public exception
{
    database_failure(const std::string& str) : exception(str) {}
};


/// Read/write access to the indexer database
namespace rw {
/**
 * \brief Class \c database w/ read/write access
 */
class database : public Xapian::WritableDatabase, public details::database
{
public:
    /// Construct from a database path
    database(dbid, const std::string&);
    virtual ~database();

    /// Access header files mapping cache (mutable)
    HeaderFilesCache& headers_map();
    /// Commit recent changes to the DB
    void commit();
};

inline HeaderFilesCache& database::headers_map()
{
    return m_files_cache;
}

}                                                           // namespace rw

/// Read-only access to the indexer database
namespace ro {
/**
 * \brief Class \c database w/ read-only access
 */
class database : public Xapian::Database, public details::database
{
public:
    /// Construct from DB path
    explicit database(const std::string&);
    /// Destructor
    ~database();
};

}}}                                                         // namespace ro, index, kate
