/**
 * \file
 *
 * \brief Class \c kate::index::kind (implementation)
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

// Project specific includes
#include <src/index/kind.h>

// Standard includes
#include <QtCore/QString>
#include <map>

namespace kate { namespace index { namespace {
std::map<kind, const char* const> SYMBOL_KIND_TO_STRING_MAP =
{
    {kind::UNEXPOSED, "<unexposed>"}
  , {kind::NAMESPACE, "namespace"}
  , {kind::NAMESPACE_ALIAS, "namespace alias"}
  , {kind::TYPEDEF, "typedef"}
  , {kind::TYPE_ALIAS, "type alias"}
  , {kind::STRUCT, "struct"}
  , {kind::CLASS, "class"}
  , {kind::UNION, "union"}
  , {kind::ENUM, "enum"}
  , {kind::ENUM_CONSTANT, "enum constant"}
  , {kind::VARIABLE, "variable"}
  , {kind::PARAMETER, "parameter"}
  , {kind::FIELD, "field"}
  , {kind::BITFIELD, "bit-field"}
  , {kind::FUNCTION, "function"}
  , {kind::METHOD, "method"}
  , {kind::CONSTRUCTOR, "constructor"}
  , {kind::DESTRUCTOR, "destructor"}
  , {kind::CONVERSTION, "converstion"}
};

template <typename String>
String to_string_impl(const kind k)
{
    auto it = SYMBOL_KIND_TO_STRING_MAP.find(k);
    assert(
        "List of symbol's kind seems changed! Review your code!"
      && it != end(SYMBOL_KIND_TO_STRING_MAP)
      && SYMBOL_KIND_TO_STRING_MAP.size() == std::size_t(kind::last__)
      && k < kind::last__
      );
    return it->second;
}
}                                                           // anonymous namespace

std::string to_string(const kind k)
{
    return to_string_impl<std::string>(k);
}

QString toString(const kind k)
{
    return to_string_impl<QString>(k);
}

}}                                                          // namespace index, kate
