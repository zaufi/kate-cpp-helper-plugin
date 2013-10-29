/**
 * \file
 *
 * \brief Commonly used types for indexing susbystem
 *
 * \date Sat Oct 26 12:26:50 MSK 2013 -- Initial design
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
#include <src/index/serialize.h>

// Standard includes
#include <xapian/types.h>
#include <cstdint>
#include <string>

namespace kate { namespace index {

typedef uint32_t dbid;
typedef Xapian::docid docid;
typedef Xapian::doccount doccount;

class docref
{
public:
    constexpr docref() : m_as_long{0} {}

    constexpr docref(const dbid id1, const docid id2)
      : m_db_id{id1}
      , m_doc_id{id2}
    {}

    dbid database_id() const
    {
        return m_db_id;
    }
    docid document_id() const
    {
        return m_doc_id;
    }

    bool is_valid() const
    {
        return m_db_id && m_doc_id;
    }

    static std::string to_string(docref ref)
    {
        return serialize(ref.m_as_long);
    }
    static docref from_string(const std::string& raw)
    {
        auto result = deserialize<decltype(docref::m_as_long)>(raw);
        return docref{result};
    }

private:
    constexpr explicit docref(uint64_t val)
      : m_as_long{val}
    {}

    union
    {
        uint64_t m_as_long;
        struct
        {
            dbid m_db_id;
            docid m_doc_id;
        };
    };
};

}}                                                          // namespace index, kate
