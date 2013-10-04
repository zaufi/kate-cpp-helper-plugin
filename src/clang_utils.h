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
# include <KDE/KUrl>
# include <QtCore/QString>
# include <QtCore/QDebug>
# include <stdexcept>

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

/**
 * \brief Class \c location
 *
 * \note For some reason `kate` has line/column as \c int,
 * so lets have this class to convert \c unsigned from \em libclang to it.
 */
class location
{
public:
    struct exception : public std::runtime_error
    {
        struct invalid;
        explicit exception(const std::string&);
    };

    location() = default;
    location(KUrl&&, int, int);                             ///< Build a location instance from components
    location(const CXIdxLoc);                               ///< Construct from \c CXIdxLoc
    location(const CXSourceLocation);                       ///< Construct from \c CXSourceLocation
    location(location&&) noexcept;                          ///< Move ctor
    location& operator=(location&&) noexcept;               ///< Move-assign operator
    location(const location&) = default;                    ///< Default copy ctor
    location& operator=(const location&) = default;         ///< Default copy-assign operator

    /// \name Accessors
    //@{
    int line() const
    {
        return m_line;
    }
    int column() const
    {
        return m_column;
    }
    int offset() const
    {
        return m_offset;
    }
    const KUrl& file() const
    {
        return m_file;
    }
    //@}

    bool empty() const
    {
        return m_file.isEmpty();
    }

private:
    KUrl m_file;
    int m_line;
    int m_column;
    int m_offset;
};

struct location::exception::invalid : public location::exception
{
    invalid(const std::string& str = "Invalid source location")
      : exception(str)
    {}
};

inline location::exception::exception(const std::string& str)
  : std::runtime_error(str)
{}

inline location::location(KUrl&& file, const int line, const int column)
  : m_line(line)
  , m_column(column)
  , m_offset(-1)
{
    m_file.swap(file);
}

inline location::location(location&& other) noexcept
  : m_line(other.m_line)
  , m_column(other.m_column)
  , m_offset(other.m_offset)
{
    m_file.swap(other.m_file);
}

inline location& location::operator=(location&& other) noexcept
{
    if (&other != this)
    {
        m_file.swap(other.m_file);
        m_line = other.m_line;
        m_column = other.m_column;
        m_offset = other.m_offset;
    }
    return *this;
}

inline QDebug operator<<(QDebug dbg, const location& l)
{
    dbg.nospace() << l.file() << ':' << l.line() << ':' << l.column();
    return dbg.space();
}

/// \name Debug streaming support for various clang types
//@{
inline QDebug operator<<(QDebug dbg, const CXFile file)
{
    DCXString filename = {clang_getFileName(file)};
    dbg.nospace() << clang_getCString(filename);
    return dbg.space();
}

#if 0
inline QDebug operator<<(QDebug dbg, const CXSourceLocation l)
{
    CXFile file;
    unsigned line;
    unsigned column;
    clang_getSpellingLocation(l, &file, &line, &column, nullptr);
    dbg.nospace() << file << ':' << line << ':' << column;
    return dbg.space();
}
#endif

inline QDebug operator<<(QDebug dbg, const CXCursor c)
{
    auto kind = clang_getCursorKind(c);
    DCXString kind_str = {clang_getCursorKindSpelling(kind)};
    DCXString csp_str = {clang_getCursorDisplayName(c)};
    dbg.nospace() << clang_getCursorLocation(c) << ": entity:" << clang_getCString(csp_str)
      << ", kind:" << clang_getCString(kind_str);
    return dbg.space();
}
//@}

/// \name Convert various clang types to \c QString
//@{

/// Get a human readable string of \c CXString
inline QString toString(const DCXString& str)
{
    return QString(clang_getCString(str));
}

/// Get a human readable string of \c CXCursorKind
inline QString toString(const CXCursorKind v)
{
    return toString(DCXString{clang_getCursorKindSpelling(v)});
}

/// Get a human readable string of \c CXFile
inline QString toString(const CXFile file)
{
    return toString(clang_getFileName(file));
}

/// Get a human readable string of \c CXCompletionChunkKind
QString toString(CXCompletionChunkKind);

/// Get a human readable string of \c CXIdxEntityKind
QString toString(CXIdxEntityKind);

/// Get a human readable string of \c CXIdxEntityCXXTemplateKind
QString toString(CXIdxEntityCXXTemplateKind);

//@}
}                                                           // namespace kate
#endif                                                      // __SRC__CLANG_UTILS_H__
// kate: hl C++11/Qt4;
