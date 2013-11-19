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
#include <src/index/document.h>

// Standard includes
#include <KDE/KDebug>
#include <xapian/database.h>
#include <xapian/query.h>
#include <xapian/queryparser.h>
#include <xapian/enquire.h>

namespace kate { namespace index {
/**
 * \attention If u r going to modify this constructor somehow,
 * make sure \c KCompletion model also modified accordingly.
 */
combined_index::combined_index()
  : m_arity_processor{value_slot::ARITY, "arity"}
  , m_size_processor{value_slot::SIZEOF, "size"}
  , m_align_processor{value_slot::ALIGNOF, "align"}
  , m_line_processor{value_slot::LINE, "line"}
  , m_column_processor{value_slot::COLUMN, "column"}
{
    m_qp.add_valuerangeprocessor(&m_arity_processor);
    m_qp.add_valuerangeprocessor(&m_align_processor);
    m_qp.add_valuerangeprocessor(&m_column_processor);
    m_qp.add_valuerangeprocessor(&m_line_processor);
    m_qp.add_valuerangeprocessor(&m_size_processor);

    // Setup prefixes
    m_qp.add_boolean_prefix("access", term::XACCESS);
    m_qp.add_boolean_prefix("anon", term::XANONYMOUS);
    m_qp.add_boolean_prefix("anonymous", term::XANONYMOUS);
    m_qp.add_boolean_prefix("base", term::XBASE_CLASS);
    m_qp.add_boolean_prefix("inh", term::XINHERITANCE);
    m_qp.add_boolean_prefix("inheritance", term::XINHERITANCE);
    m_qp.add_boolean_prefix("decl", term::XDECL);
    m_qp.add_boolean_prefix("kind", term::XKIND);
    m_qp.add_boolean_prefix("pod", term::XPOD);
    m_qp.add_boolean_prefix("def", term::XREDECLARATION);
    m_qp.add_boolean_prefix("ref", term::XREF);
    m_qp.add_boolean_prefix("scope", term::XSCOPE);
    m_qp.add_boolean_prefix("static", term::XSTATIC);
    m_qp.add_boolean_prefix("template", term::XTEMPLATE);
    m_qp.add_boolean_prefix("virtual", term::XVIRTUAL);
}

std::vector<document> combined_index::search(
    const QString& q
  , const doccount start
  , const doccount maxitems
  )
{
    recombine_database();                                   // Make sure DB is Ok
    assert("Sanity check" && m_compound_db);
    kDebug(DEBUG_AREA) << "Indices enabled: " << m_db_list.size();
    if (m_db_list.empty())
    {
        throw std::runtime_error("No indices enabled for search...");
    }
    //
    auto query_str = std::string{q.toUtf8().constData()};
    Xapian::Query query = parse_query(query_str);
    kDebug(DEBUG_AREA) << "Parsed query: " << query.get_description().c_str();
    //
    Xapian::MSet matches;
    try
    {
        auto enquire = Xapian::Enquire{*m_compound_db};     // NOTE May throw only if DB instance is uninitialized
        enquire.set_query(query);
        enquire.set_sort_by_relevance();
        matches = enquire.get_mset(start, maxitems);
    }
    catch (const Xapian::Error& e)
    {
        throw std::runtime_error(std::string{"Database failure: "} + e.get_msg());
    }
    kDebug(DEBUG_AREA) <<  "Documents found:" << matches.size();
    kDebug(DEBUG_AREA) << "Documents estimated:" << matches.get_matches_estimated();
    //
    auto result = std::vector<document>{};
    for (auto it = std::begin(matches), last = std::end(matches); it != last; ++it)
        result.emplace_back(it.get_document());
    return result;
}

void combined_index::add_index(ro::database* ptr)
{
    auto it = std::find(begin(m_db_list), end(m_db_list), ptr);
    if (it == end(m_db_list))
    {
        recombine_database();
        m_db_list.emplace_back(ptr);
        m_compound_db->add_database(*ptr);
        kDebug(DEBUG_AREA) << "add index to search:" << ptr->id() << ":" << m_db_list.size();
    }
}

void combined_index::remove_index(ro::database* ptr)
{
    auto it = std::find(begin(m_db_list), end(m_db_list), ptr);
    if (it != end(m_db_list))
    {
        m_db_list.erase(it);
        m_compound_db.reset();
        kDebug(DEBUG_AREA) << "remove index from search:" << ptr->id() << ":" << m_db_list.size();
    }
}

void combined_index::recombine_database()
{
    if (!m_compound_db)
    {
        m_compound_db.reset(new Xapian::Database{});
        for (auto* ptr : m_db_list)
            m_compound_db->add_database(*ptr);
    }
}

Xapian::Query combined_index::parse_query(const std::string& query_str)
{
    assert("Sanity check" && m_compound_db);

    m_qp.set_database(*m_compound_db);

    // Parse it!
    Xapian::Query query;
    try
    {
        query = m_qp.parse_query(
            query_str
          , Xapian::QueryParser::FLAG_BOOLEAN
            | Xapian::QueryParser::FLAG_WILDCARD
            | Xapian::QueryParser::FLAG_LOVEHATE
          );
    }
    catch (const Xapian::QueryParserError& e)
    {
        throw std::runtime_error(std::string{"Invalid query: "} + e.get_msg());
    }
    return query;
}

}}                                                          // namespace index, kate
