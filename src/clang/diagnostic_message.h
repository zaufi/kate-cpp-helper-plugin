/**
 * \file
 *
 * \brief Class \c kate::clang::diagnostic_message (interface)
 *
 * \date Fri Nov  8 21:14:58 MSK 2013 -- Initial design
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
#include <src/clang/location.h>

// Standard includes
#include <QtCore/QString>
#include <cassert>

namespace kate { namespace clang {

/**
 * Structure to hold info about diagnostic message
 *
 * \note Qt4 has a problem w/ rvalue references in signal/slots,
 * so default copy ctor and assign operator needed :(
 */
struct diagnostic_message
{
    /// Severity level
    enum class type
    {
        debug
      , info
      , warning
      , error
      , cutset
    };

    location m_location;                                    ///< Location in source code
    QString m_text;                                         ///< Diagnostic message text
    type m_type;                                            ///< Type of the record

    /// \c record class must be default constructible
    diagnostic_message() = default;
    /// Make a \c record from parts
    diagnostic_message(location&&, QString&&, type) noexcept;
    /// Make a \c record w/ message and given type (and empty location)
    diagnostic_message(const QString&, type) noexcept;
    /// Make a \c record w/ message and given type (and empty location)
    diagnostic_message(QString&&, type) noexcept;
    /// Move ctor
    diagnostic_message(diagnostic_message&&) noexcept;
    /// Move-assign operator
    diagnostic_message& operator=(diagnostic_message&&) noexcept;
    /// Default copy ctor
    diagnostic_message(const diagnostic_message&) = default;
    /// Default copy-assign operator
    diagnostic_message& operator=(const diagnostic_message&) = default;
};

inline diagnostic_message::diagnostic_message(
    const QString& text
  , diagnostic_message::type type
  ) noexcept
  : m_text(text)
  , m_type(type)
{
}

inline diagnostic_message::diagnostic_message(
    QString&& text
  , diagnostic_message::type type
  ) noexcept
  : m_type(type)
{
    m_text.swap(text);
}

inline diagnostic_message::diagnostic_message(
    location&& loc
  , QString&& text
  , const diagnostic_message::type type
  ) noexcept
  : m_location(std::move(loc))
  , m_type(type)
{
    m_text.swap(text);
}

inline diagnostic_message::diagnostic_message(diagnostic_message&& other) noexcept
  : m_location(std::move(other.m_location))
  , m_type(other.m_type)
{
    m_text.swap(other.m_text);
}

inline auto diagnostic_message::operator=(diagnostic_message&& other) noexcept -> diagnostic_message&
{
    assert("R U INSANE? ;-)" && &other != this);
    m_location = std::move(other.m_location);
    m_text.swap(other.m_text);
    m_type = other.m_type;
    return *this;
}

}}                                                          // namespace clang, kate
