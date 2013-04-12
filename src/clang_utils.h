/**
 * \file
 *
 * \brief Some helpers to deal w/ clang API
 *
 * \date Mon Nov 19 04:31:41 MSK 2012 -- Initial design
 */
/*
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

#ifndef __SRC__CLANG_UTILS_H__
# define __SRC__CLANG_UTILS_H__

// Project specific includes

// Standard includes
# include <clang-c/Index.h>
# include <QtCore/QString>
# include <QtCore/QDebug>

namespace kate {
/// RAII helper to release various clang resources
template <typename T, void(*DisposeFn)(T)>
class disposable
{
public:
    disposable(T value) : m_value(value) {}
    ~disposable()
    {
        DisposeFn(m_value);
    }
    operator T() const
    {
        return m_value;
    }

private:
    T m_value;
};
/// RAII helper to release various clang resources spicified as pointers
template <typename T, void(*DisposeFn)(T*)>
class disposable<T*, DisposeFn>
{
public:
    disposable(T* value) : m_value(value) {}
    ~disposable()
    {
        DisposeFn(m_value);
    }
    operator T*() const
    {
        return m_value;
    }
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

/// Get a human readable string of \c CXCursorKind
inline QString toString(const CXCursorKind v)
{
    DCXString str = clang_getCursorKindSpelling(v);
    return QString(clang_getCString(str));
}

/// Get a human readable string of \c CXCompletionChunkKind
QString toString(const CXCompletionChunkKind);

inline QDebug operator<<(QDebug dbg, const CXSourceLocation& l)
{
    CXFile file;
    unsigned line;
    unsigned column;
    clang_getSpellingLocation(l, &file, &line, &column, nullptr);
    DCXString filename = clang_getFileName(file);
    dbg.nospace() << clang_getCString(filename) << ':' << line << ':' << column;
    return dbg.space();
}

inline QDebug operator<<(QDebug dbg, const CXCursor& c)
{
    auto kind = clang_getCursorKind(c);
    DCXString kind_str = clang_getCursorKindSpelling(kind);
    DCXString csp_str = clang_getCursorDisplayName(c);
    dbg.nospace() << clang_getCursorLocation(c) << ": entity:" << clang_getCString(csp_str)
      << ", kind:" << clang_getCString(kind_str);
    return dbg.space();
}

}                                                           // namespace kate
#endif                                                      // __SRC__CLANG_UTILS_H__
// kate: hl C++11/Qt4;
