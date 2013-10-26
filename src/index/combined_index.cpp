/**
 * \file
 *
 * \brief Class \c kate::index::combined_index (implementation)
 *
 * \date Sat Oct 26 11:22:08 MSK 2013 -- Initial design
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
#include <src/index/combined_index.h>
#include <src/index/database.h>

// Standard includes
#include <xapian/database.h>

namespace kate { namespace index {

std::vector<search_result> combined_index::search(const QString&)
{
    auto result = std::vector<search_result>{};
    return result;
}

void combined_index::add_index(ro::database* ptr)
{
    auto it = std::find(begin(m_db_list), end(m_db_list), ptr);
    if (it == end(m_db_list))
    {
        m_db_list.emplace_back(ptr);
        m_compound_db.reset();
    }

}
void combined_index::remove_index(ro::database* ptr)
{
    auto it = std::find(begin(m_db_list), end(m_db_list), ptr);
    if (it != end(m_db_list))
    {
        m_db_list.erase(it);
        m_compound_db.reset();
    }
}

void combined_index::recombine_database()
{
    m_compound_db.reset(new Xapian::Database{});
    for (auto* ptr : m_db_list)
        m_compound_db->add_database(*ptr);
}

}}                                                          // namespace index, kate
