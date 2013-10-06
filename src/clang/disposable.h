/**
 * \file
 *
 * \brief Class \c kate::clang::disposable (interface)
 *
 * \date Sun Oct  6 22:26:42 MSK 2013 -- Initial design
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
#include <clang-c/Index.h>
#include <stdexcept>

namespace kate { namespace clang {

/**
 * \brief RAII helper to release various clang resources
 */
template <typename T, void(*DisposeFn)(T)>
class disposable
{
public:
    disposable(T value) : m_value(value) {}
    ~disposable()
    {
        DisposeFn(m_value);
    }
    /// Delete copy ctor
    disposable(const disposable&) = delete;
    /// Delete copy-assign operator
    disposable& operator=(const disposable&) = delete;
    /// Implicit conversion to \c T
    operator T() const
    {
        return m_value;
    }

private:
    T m_value;
};

/**
 * \brief RAII helper to release various clang resources spicified as pointers
 */
template <typename T, void(*DisposeFn)(T*)>
class disposable<T*, DisposeFn>
{
public:
    disposable(T* value) : m_value(value) {}
    ~disposable()
    {
        DisposeFn(m_value);
    }
    /// Delete move ctor
    disposable(disposable&&) = delete;
    /// Delete move-assign operator
    disposable& operator=(disposable&&) = delete;
    /// Implicit conversion to \c T*
    operator T*() const
    {
        return m_value;
    }
    /// Get access to underlaid pointer
    T* operator ->() const
    {
        return m_value;
    }

private:
    T* m_value;
};

typedef disposable<CXIndex, &clang_disposeIndex> DCXIndex;
typedef disposable<CXString, &clang_disposeString> DCXString;
typedef disposable<CXTranslationUnit, &clang_disposeTranslationUnit> DCXTranslationUnit;
typedef disposable<CXDiagnostic, &clang_disposeDiagnostic> DCXDiagnostic;
typedef disposable<CXIndexAction, &clang_IndexAction_dispose> DCXIndexAction;
typedef disposable<CXCodeCompleteResults*, &clang_disposeCodeCompleteResults> DCXCodeCompleteResults;

}}                                                          // namespace clang, kate
