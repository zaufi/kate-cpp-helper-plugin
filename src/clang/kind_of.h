/**
 * \file
 *
 * \brief A set of overloads of \c kate::clang::kind_of for various clang types
 *
 * \date Sun Oct  6 23:31:41 MSK 2013 -- Initial design
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

namespace kate { namespace clang {

/// \name Functions to get kind of various clang entities in a unified way
//@{

inline CXCursorKind kind_of(const CXCursor& c)
{
    return clang_getCursorKind(c);
}

inline CXIdxEntityKind kind_of(const CXIdxEntityInfo& e)
{
    return e.kind;
}

inline CXTypeKind kind_of(const CXType& t)
{
    return t.kind;
}

inline CXCompletionChunkKind kind_of(CXCompletionString completion_string, unsigned chunk_number)
{
    return clang_getCompletionChunkKind(completion_string, chunk_number);
}
//@}

/**
 * \c CXIdxEntityCXXTemplateKind has meaning only for a limited set of \c CXIdxEntityKind value.
 * This function return \c true if \c CXIdxEntityInfo::templateKind member can be accessed,
 * \c false otherwise.
 */
inline bool may_apply_template_kind(const CXIdxEntityKind kind)
{
    switch (kind)
    {
        case CXIdxEntity_Function:
        case CXIdxEntity_CXXClass:
        case CXIdxEntity_CXXStaticMethod:
        case CXIdxEntity_CXXInstanceMethod:
        case CXIdxEntity_CXXConstructor:
        case CXIdxEntity_CXXConversionFunction:
        case CXIdxEntity_CXXTypeAlias:
            return true;
        default:
            break;
    }
    return false;
}

}}                                                          // namespace clang, kate
