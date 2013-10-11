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

// Standard includes
#include <xapian/database.h>
#include <xapian/error.h>
#include <stdexcept>

namespace kate { namespace index {

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
    /// Construct from a database path
    explicit database(const std::string&);
    /// Destructor
    virtual ~database() {}
};

struct database::exception::failure : public database::exception
{
    failure(const std::string& str) : database::exception(str) {}
};

inline database::exception::exception(const std::string& str)
  : std::runtime_error(str) {}

inline database::database(const std::string& path) try
  : Xapian::WritableDatabase(path, Xapian::DB_CREATE_OR_OPEN)
{
}
catch (const Xapian::DatabaseError& e)
{
    throw database::exception::failure("Index database [" + path + "] failure: " + e.get_msg());
}

}}                                                          // namespace index, kate
