/**
 * \file
 *
 * \brief Class \c kate::index::details::database (interface)
 *
 * \date Sun Nov  3 19:00:54 MSK 2013 -- Initial design
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

namespace kate { namespace index { namespace details {

/**
 * \brief Base class for indexer databases
 *
 * This class have some common members and provide some basic methods to access them.
 */
class database
{
public:
    explicit database(const dbid db_id = 0) : m_id(db_id) {}

    /// Access header files mapping cache (immutable)
    const HeaderFilesCache& headers_map() const
    {
        return m_files_cache;
    }
    /// Get (short) database ID
    dbid id() const
    {
        return m_id;
    }

protected:
    HeaderFilesCache m_files_cache;
    dbid m_id;
};

}}}                                                         // namespace details, index, kate
