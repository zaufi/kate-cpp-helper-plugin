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

namespace kate { namespace index { namespace term {
const std::string XDECL = "XDCL";
const std::string XREF = "XRF";
const std::string XCONTAINER = "XCNT";
const std::string XREDECLARATION = "XRDL";
}                                                           // namespace term

namespace {
const std::string FILES_MAPPING = "HDRMAPCACHE";
}                                                           // anonymous namespace

namespace details {
template <typename Database>
inline database<Database>::database()
{
    auto hdr_cache = static_cast<Database* const>(this)->get_metadata(FILES_MAPPING);
    if (!hdr_cache.empty())
        m_files_cache.loadFromString(hdr_cache);
}
}                                                           // namespace details


namespace rw {

database::database(const std::string& path) try
  : Xapian::WritableDatabase(path, Xapian::DB_CREATE_OR_OPEN)
  , details::database<database>()
{
}
catch (const Xapian::DatabaseError& e)
{
    throw exception::database_failure("Index database [" + path + "] failure: " + e.get_msg());
}

database::~database()
{
    commit();
}

void database::commit()
{
    try
    {
        kDebug(DEBUG_AREA) << "Commiting DB changes...";
        if (headers_map().isDirty())
            set_metadata(FILES_MAPPING, headers_map().storeToString());
        Xapian::WritableDatabase::commit();
    }
    catch (...)
    {
        kDebug(DEBUG_AREA) << "Fail to store headers mapping cache";
    }
}

}                                                           // namespace rw

namespace ro {

database::database(const std::string& path) try
  : Xapian::Database(path)
  , details::database<database>()
{
}
catch (const Xapian::DatabaseError& e)
{
    throw exception::database_failure("Index database [" + path + "] failure: " + e.get_msg());
}

database::~database()
{
}

}}}                                                         // namespace ro, index, kate
