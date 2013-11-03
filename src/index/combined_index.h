/**
 * \file
 *
 * \brief Class \c kate::index::combined_index (interface)
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

#pragma once

// Project specific includes
#include <src/index/types.h>
#include <src/index/numeric_value_range_processor.h>

// Standard includes
#include <memory>
#include <vector>

class QString;

namespace Xapian {
class Database;
class Document;
class Query;
}                                                           // namespace Xapian

namespace kate { namespace index { namespace ro {
class database;
}                                                           // namespace ro
class document;


/**
 * \brief Compound searchable database
 *
 * [More detailed description here]
 *
 */
class combined_index
{
public:
    /// Iniialize query parser w/ range processors and prefixes
    combined_index();
    /// Search over all connected indices
    std::vector<document> search(const QString&, doccount = 0, doccount = 2000);

    void add_index(ro::database*);
    void remove_index(ro::database*);

private:
    void recombine_database();
    Xapian::Query parse_query(const std::string&);

    std::vector<ro::database*> m_db_list;
    std::unique_ptr<Xapian::Database> m_compound_db;
    numeric_value_range_processor m_arity_processor;
    numeric_value_range_processor m_size_processor;
    numeric_value_range_processor m_align_processor;
    numeric_value_range_processor m_line_processor;
    numeric_value_range_processor m_column_processor;
    Xapian::QueryParser m_qp;
};

}}                                                          // namespace index, kate
