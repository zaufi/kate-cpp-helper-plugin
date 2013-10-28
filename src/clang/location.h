/**
 * \file
 *
 * \brief Class \c kate::clang::location (interface)
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

#pragma once

// Project specific includes

// Standard includes
#include <clang-c/Index.h>
#include <KUrl>
#include <QtCore/QDebug>
#include <ostream>
#include <stdexcept>
#include <string>

namespace kate { namespace clang {

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
    explicit location(const CXCursor&);                     ///< Construct from \c CXCursor
    location(location&&) noexcept;                          ///< Move ctor
    location& operator=(location&&) noexcept;               ///< Move-assign operator
    location(const location&) = default;                    ///< Default copy ctor
    location& operator=(const location&) = default;         ///< Default copy-assign operator

    /// \name Accessors/observers
    //@{
    int line() const;
    int column() const;
    int offset() const;
    const KUrl& file() const;
    bool empty() const;
    //@}

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

/// \note Delegating constructors require gcc >= 4.7
/// \todo Need workaround for gcc < 4.7. Really?
inline location::location(const CXCursor& cursor)
  : location{clang_getCursorLocation(cursor)}
{
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

inline int location::line() const
{
    return m_line;
}

inline int location::column() const
{
    return m_column;
}

inline int location::offset() const
{
    return m_offset;
}

inline const KUrl& location::file() const
{
    return m_file;
}

inline bool location::empty() const
{
    return m_file.isEmpty();
}

/// Make \c location printable to Qt debug streams
inline QDebug operator<<(QDebug dbg, const location& l)
{
    if (!l.empty())
        dbg.nospace() << l.file().toLocalFile() << ':' << l.line() << ':' << l.column();
    else
        dbg.nospace() << "<empty-location>";
    return dbg.space();
}

/// Make \c location printable to C++ streams
inline std::ostream& operator<<(std::ostream& os, const location& l)
{
    os << l.file().toLocalFile().toUtf8().constData() << ':' << l.line() << ':' << l.column();
    return os;
}

}}                                                          // namespace clang, kate

Q_DECLARE_METATYPE(kate::clang::location);

