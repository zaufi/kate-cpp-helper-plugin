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
#include <src/header_files_cache.h>

// Standard includes
#include <xapian/database.h>
#include <xapian/error.h>
#include <stdexcept>
#include <string>

namespace kate { namespace index { namespace term {
extern const std::string XDECL;
extern const std::string XREF;
extern const std::string XCONTAINER;
extern const std::string XREDECLARATION;
}                                                           // namespace term
namespace value_slot {
constexpr Xapian::valueno LINE = 1;
constexpr Xapian::valueno COLUMN = 2;
constexpr Xapian::valueno FILE = 3;
constexpr Xapian::valueno SEMANTIC_CONTAINER = 4;
constexpr Xapian::valueno LEXICAL_CONTAINER = 5;
constexpr Xapian::valueno KIND = 6;
constexpr Xapian::valueno TYPE = 7;
}                                                           // namespace value_slot

/// Exceptions group for database classes
struct exception : public std::runtime_error
{
    struct database_failure;
    explicit exception(const std::string&);
};

constexpr Xapian::docid IVALID_DOCUMENT_ID = 0u;            ///< Value to indecate invalid document

struct exception::database_failure : public exception
{
    database_failure(const std::string& str) : exception(str) {}
};

inline exception::exception(const std::string& str)
  : std::runtime_error(str) {}

namespace details {
/**
 * \brief Base class for indexer databases
 *
 * This class have some common members and provide some basic methods to access them.
 */
template <typename Database>
class database
{
public:
    database();
    HeaderFilesCache& headers_map();                        ///< Access header files mapping cache (mutable)
    const HeaderFilesCache& headers_map() const;            ///< Access header files mapping cache (immutable)

private:
    HeaderFilesCache m_files_cache;
};

template <typename Database>
inline HeaderFilesCache& database<Database>::headers_map()
{
    return m_files_cache;
}

template <typename Database>
inline const HeaderFilesCache& database<Database>::headers_map() const
{
    return m_files_cache;
}
}                                                           // namespace details

/// Read/write access to the indexer database
namespace rw {
/**
 * \brief Class \c database w/ read/write access
 */
class database : public Xapian::WritableDatabase, public details::database<database>
{
public:
    explicit database(const std::string&);                  ///< Construct from a database path
    virtual ~database();

    void commit();                                          ///< Commit recent changes to the DB
};
}                                                           // namespace rw

/// Read-only access to the indexer database
namespace ro {
/**
 * \brief Class \c database w/ read-only access
 */
class database : public Xapian::Database, public details::database<database>
{
public:
    /// Default constructor
    explicit database(const std::string&);
    /// Destructor
    ~database();
};

}}}                                                         // namespace ro, index, kate
