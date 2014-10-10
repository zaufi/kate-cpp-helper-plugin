/**
 * \file
 *
 * \brief Bunch of overloads to convert various clang types to \c QString or \c std::string
 *
 * \date Sun Oct  6 23:31:48 MSK 2013 -- Initial design
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

// Standard includes
#include <QtCore/QString>
#include <string>

namespace kate { namespace clang {

/// \name Convert various clang types to \c QString and \c std::string
//@{

/// Get a human readable string of \c CXString
inline QString toString(const DCXString& str)
{
    auto* cstr = clang_getCString(str);
    if (cstr)
        return QString{cstr};
    return QString{};
}
inline std::string to_string(const DCXString& str)
{
    auto* cstr = clang_getCString(str);
    if (cstr)
        return std::string{cstr};
    return std::string{};
}

/// Get a human readable string of \c CXFile
inline QString toString(const CXFile file)
{
    return toString(DCXString{clang_getFileName(file)});
}
inline std::string to_string(const CXFile file)
{
    return to_string(DCXString{clang_getFileName(file)});
}

/// Get a human readable string of \c CXCursorKind
inline QString toString(const CXCursorKind v)
{
    return toString(DCXString{clang_getCursorKindSpelling(v)});
}
inline std::string to_string(const CXCursorKind v)
{
    return to_string(DCXString{clang_getCursorKindSpelling(v)});
}

/// Get a human readable string of \c CXType
inline QString toString(const CXType t)
{
    return toString(DCXString{clang_getTypeSpelling(t)});
}
inline std::string to_string(const CXType t)
{
    return to_string(DCXString{clang_getTypeSpelling(t)});
}

/// Get a human readable string of \c CXTypeKind
inline QString toString(const CXTypeKind t)
{
    return toString(DCXString{clang_getTypeKindSpelling(t)});
}
inline std::string to_string(const CXTypeKind t)
{
    return to_string(DCXString{clang_getTypeKindSpelling(t)});
}

/// Get a human readable string of \c CXCompletionChunkKind
QString toString(CXCompletionChunkKind);
std::string to_string(CXCompletionChunkKind);

/// Get a human readable string of \c CXIdxEntityKind
QString toString(CXIdxEntityKind);
std::string to_string(CXIdxEntityKind);

/// Get a human readable string of \c CXIdxEntityCXXTemplateKind
QString toString(CXIdxEntityCXXTemplateKind);
std::string to_string(CXIdxEntityCXXTemplateKind);

/// Get a human readable string of \c CXLinkageKind
QString toString(CXLinkageKind);
std::string to_string(CXLinkageKind);

//@}

}}                                                          // namespace clang, kate
