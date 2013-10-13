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

/**
 * \brief [Type brief class description here]
 *
 * [More detailed description here]
 *
 */
class database : public Xapian::WritableDatabase
{
public:
    struct exception : public std::runtime_error
    {
        struct failure;
        explicit exception(const std::string&);
    };

    explicit database(const std::string&);                  ///< Construct from a database path
    virtual ~database();

    HeaderFilesCache& headers_map();                        ///< Access header files mapping cache (mutable)
    const HeaderFilesCache& headers_map() const;            ///< Access header files mapping cache (immutable)
    void commit();                                          ///< Commit recent changes to the DB

    static constexpr Xapian::docid IVALID_DOCUMENT_ID = 0u; ///< Value to indecate invalid document

private:
    HeaderFilesCache m_files_cache;
};

struct database::exception::failure : public database::exception
{
    failure(const std::string& str) : database::exception(str) {}
};

inline database::exception::exception(const std::string& str)
  : std::runtime_error(str) {}

inline HeaderFilesCache& database::headers_map()
{
    return m_files_cache;
}

inline const HeaderFilesCache& database::headers_map() const
{
    return m_files_cache;
}

}}                                                          // namespace index, kate
