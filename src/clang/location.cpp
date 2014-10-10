/**
 * \file
 *
 * \brief Class \c kate::clang::location (implementation)
 *
 * \date Sun Oct  6 23:23:51 MSK 2013 -- Initial design
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
#include "location.h"
#include "disposable.h"

// Standard includes
#include <cassert>

namespace kate { namespace clang {

location::location(const CXIdxLoc loc)
{
    CXIdxClientFile file;
    unsigned line;
    unsigned column;
    unsigned offset;
    clang_indexLoc_getFileLocation(loc, &file, nullptr, &line, &column, &offset);
    if (line == 0)
        throw exception::invalid();
    if (file == nullptr)
        throw exception::invalid("No index file has attached to a source location");
    DCXString filename = {clang_getFileName(static_cast<CXFile>(file))};
    m_file = clang_getCString(filename);
    assert("Sanity check" && m_file.isValid() && !m_file.isEmpty());
    m_file.cleanPath();
    m_line = line;
    m_column = column;
    m_offset = offset;
}

location::location(const CXSourceLocation loc)
{
    CXFile file;
    unsigned line;
    unsigned column;
    unsigned offset;
    clang_getSpellingLocation(loc, &file, &line, &column, &offset);
    if (file == nullptr)
        throw exception::invalid("No file has attached to a source location");
    DCXString filename = {clang_getFileName(static_cast<CXFile>(file))};
    m_file = clang_getCString(filename);
    assert("Sanity check" && m_file.isValid() && !m_file.isEmpty());
    m_file.cleanPath();
    m_line = line;
    m_column = column;
    m_offset = offset;
}

}}                                                          // namespace clang, kate
