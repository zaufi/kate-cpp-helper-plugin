/**
 * \file
 *
 * \brief Class \c kate::clang::compiler_options (interface)
 *
 * \date Fri Oct 25 05:11:29 MSK 2013 -- Initial design
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
#include <QtCore/QByteArray>
#include <deque>
#include <iosfwd>
#include <vector>

class QDebug;
class QString;
class QStringList;

namespace kate { namespace clang {

class compiler_options;

/// Make \c location printable to Qt debug streams
QDebug operator<<(QDebug, const compiler_options&);

/// Make \c location printable to C++ streams
std::ostream& operator<<(std::ostream&, const compiler_options&);

/**
 * \brief A wrapper class to hide ugly underlaid details of compiler options.
 *
 * The origin of a "problem": Qt uses \c QString to manage strings, which is UCS4 encoded.
 * Clang-c API uses (surprise) plain C arrays. So to convert a \c QStringList (a typical way
 * to hold a list of strings) to a required form, some dirty work has to be done...
 *
 */
class compiler_options
{
public:
    /// Default constructor
    compiler_options() = default;
    /// Make a "shadow" copy of given params
    explicit compiler_options(const QStringList&);
    /// Default move ctor
    compiler_options(compiler_options&&) = default;
    /// Default move-assign operator
    compiler_options& operator=(compiler_options&&) = default;
    /// Delete copy ctor
    compiler_options(const compiler_options&) = delete;
    /// Delete copy-assign operator
    compiler_options& operator=(const compiler_options&) = delete;

    /// Get options understandable by clang-c API
    std::vector<const char*> get() const;

    /// Append option
    compiler_options& operator<<(const QString&);

private:
    friend QDebug operator<<(QDebug, const compiler_options&);
    friend std::ostream& operator<<(std::ostream&, const compiler_options&);

    std::deque<QByteArray> m_shadow;
};

}}                                                          // namespace clang, kate
