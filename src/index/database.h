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
#include <src/index/types.h>
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
extern const std::string XKIND;
extern const std::string XANONYMOUS;
extern const std::string XSTATIC;
extern const std::string XSCOPE;
extern const std::string XTEMPLATE;
}                                                           // namespace term

enum class value_slot : Xapian::valueno
{
    NAME
  , LINE
  , COLUMN
  , FILE
  , SEMANTIC_CONTAINER
  , LEXICAL_CONTAINER
  , TYPE
  , DBID
  , KIND
  , TEMPLATE
  , STATIC
  , SIZEOF
  , ALIGNOF
  , VALUE
};

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
class common_base
{
public:
    explicit common_base(const dbid db_id = 0) : m_id(db_id) {}

    const HeaderFilesCache& headers_map() const;            ///< Access header files mapping cache (immutable)
    dbid id() const;

protected:
    HeaderFilesCache m_files_cache;
    dbid m_id;
};

inline const HeaderFilesCache& common_base::headers_map() const
{
    return m_files_cache;
}

inline dbid common_base::id() const
{
    return m_id;
}

}                                                           // namespace details

/**
 * \brief Class \c document
 */
class document : public Xapian::Document
{
public:
    /// Defaulted default ctor
    document() = default;
    /// Conversion ctor from \c Xapian::Document
    document(Xapian::Document d) : Xapian::Document{d} {}
    /// Default copy ctor
    document(const document&) = default;
    /// Default copy-assign operator
    document& operator=(const document&) = default;

    void add_value(const value_slot slot, const std::string& value)
    {
        Xapian::Document::add_value(Xapian::valueno(slot), value);
    }
    std::string get_value(const value_slot slot) const
    {
        return Xapian::Document::get_value(Xapian::valueno(slot));
    }
};

/// Read/write access to the indexer database
namespace rw {
/**
 * \brief Class \c database w/ read/write access
 */
class database : public Xapian::WritableDatabase, public details::common_base
{
public:
    database(dbid, const std::string&);                     ///< Construct from a database path
    virtual ~database();

    HeaderFilesCache& headers_map();                        ///< Access header files mapping cache (mutable)

    void commit();                                          ///< Commit recent changes to the DB
    void add_value(value_slot, const std::string&);         ///< Add a value to a slot
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
class database : public Xapian::Database, public details::common_base
{
public:
    /// Construct from DB path
    explicit database(const std::string&);
    /// Destructor
    ~database();
};

}}}                                                         // namespace ro, index, kate
