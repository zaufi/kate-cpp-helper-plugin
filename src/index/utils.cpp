/**
 * \file
 *
 * \brief Class \c kate::index::utils (implementation)
 *
 * \date Sat Oct 26 12:53:39 MSK 2013 -- Initial design
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
#include <src/index/utils.h>

// Standard includes

namespace kate { namespace index {

dbid make_dbid(const boost::uuids::uuid& uuid)
{
    union
    {
        dbid id;
        char buf[sizeof(dbid)];
    };
    for (auto i = 0u; i < sizeof(buf); ++i)
        buf[i] = uuid.data[i];
    return id;
}

}}                                                          // namespace index, kate
