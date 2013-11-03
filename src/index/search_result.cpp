/**
 * \file
 *
 * \brief Class \c kate::search_result (implementation)
 *
 * \date Sat Nov  2 17:23:30 MSK 2013 -- Initial design
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
#include <src/index/search_result.h>

// Standard includes
#include <cassert>
#include <map>

namespace kate { namespace index {

search_result::search_result(search_result&& other) noexcept
  : m_line{other.m_line}
  , m_column{other.m_column}
  , m_kind{other.m_kind}
  , m_template_kind{other.m_template_kind}
{
    m_name.swap(other.m_name);
    m_type.swap(other.m_type);
    m_file.swap(other.m_file);
    m_bases.swap(other.m_bases);
    m_value.swap(other.m_value);
    m_sizeof.swap(other.m_sizeof);
    m_alignof.swap(other.m_alignof);
    m_offsetof.swap(other.m_offsetof);
    m_flags.m_flags_as_int = other.m_flags.m_flags_as_int;
}

search_result& search_result::operator=(search_result&& other) noexcept
{
    assert("R u insane? ;-)" && &other != this);
    m_name.swap(other.m_name);
    m_type.swap(other.m_type);
    m_file.swap(other.m_file);
    m_bases.swap(other.m_bases);
    m_value.swap(other.m_value);
    m_sizeof.swap(other.m_sizeof);
    m_alignof.swap(other.m_alignof);
    m_offsetof.swap(other.m_offsetof);
    m_line = other.m_line;
    m_column = other.m_column;
    m_kind = other.m_kind;
    m_template_kind = other.m_template_kind;
    m_flags.m_flags_as_int = other.m_flags.m_flags_as_int;
    return *this;
}

}}                                                          // namespace index, kate
