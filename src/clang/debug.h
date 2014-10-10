/**
 * \file
 *
 * \brief Class \c kate::clang::debug (interface)
 *
 * \date Sun Oct  6 23:31:57 MSK 2013 -- Initial design
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
#include "disposable.h"
#include "kind_of.h"
#include "location.h"

// Standard includes
#include <QtCore/QDebug>

namespace kate { namespace clang {

/// \name Debug streaming support for various clang types
//@{

inline QDebug operator<<(QDebug dbg, const CXFile file)
{
    DCXString filename = {clang_getFileName(file)};
    dbg.nospace() << clang_getCString(filename);
    return dbg.space();
}

inline QDebug operator<<(QDebug dbg, const DCXString& c)
{
    dbg.nospace() << clang_getCString(c);
    return dbg.space();
}

inline QDebug operator<<(QDebug dbg, const CXCursor& c)
{
    auto kind = kind_of(c);
    DCXString csp_str = {clang_getCursorDisplayName(c)};
    dbg.nospace() << location(c) << ": entity:" << csp_str << ", kind:" << toString(kind);
    return dbg.space();
}
//@}

}}                                                          // namespace clang, kate
