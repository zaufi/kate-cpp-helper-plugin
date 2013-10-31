/**
 * \file
 *
 * \brief Enum \c kate::index::kind (interface)
 *
 * \date Wed Oct 30 17:54:10 MSK 2013 -- Initial design
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

namespace kate { namespace index {

/**
 * \brief Enumeration type for possible symbol's kind
 */
enum class kind
{
    NAMESPACE
  , NAMESPACE_ALIAS
  , TYPEDEF
  , TYPE_ALIAS
  , STRUCT
  , CLASS
  , UNION
  , ENUM
  , ENUM_CONSTANT
  , VARIABLE
  , FIELD
  , PARAMETER
  , FUNCTION
  , METHOD
  , CONSTRUCTOR
  , DESTRUCTOR
  , CONVERSTION
  , last__
};

inline std::string serialize(const kind value)
{
    return serialize(static_cast<unsigned>(value));
}

inline kind deserialize(const std::string& raw)
{
    return static_cast<kind>(deserialize<unsigned>(raw));
}

}}                                                          // namespace index, kate
