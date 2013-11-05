/**
 * \file
 *
 * \brief Class \c kate::index::document_extras (interface)
 *
 * \date Sun Nov  3 20:39:00 MSK 2013 -- Initial design
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

// Standard includes
#include <xapian/types.h>
#include <string>

namespace kate { namespace index { namespace term {
extern const std::string XACCESS;
extern const std::string XANONYMOUS;
extern const std::string XBASE_CLASS;
extern const std::string XDECL;
extern const std::string XIMPLICIT;
extern const std::string XINHERITANCE;
extern const std::string XKIND;
extern const std::string XPOD;
extern const std::string XREDECLARATION;
extern const std::string XREF;
extern const std::string XSCOPE;
extern const std::string XSTATIC;
extern const std::string XTEMPLATE;
}                                                           // namespace term

enum class value_slot : Xapian::valueno
{
    ACCESS
  , ALIGNOF
  , ARITY
  , BASES
  , COLUMN
  , DBID
  , FILE
  , FLAGS
  , KIND
  , LEXICAL_CONTAINER
  , LINE
  , NAME
  , SCOPE
  , SEMANTIC_CONTAINER
  , SIZEOF
  , TEMPLATE
  , TYPE
  , VALUE
};

}}                                                          // namespace index, kate
