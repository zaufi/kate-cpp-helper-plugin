/**
 * \file
 *
 * \brief Helper functions to convert \c QString to/from \c std::string
 *
 * \date Thu Oct 31 16:46:12 MSK 2013 -- Initial design
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
#include "clang/disposable.h"

// Standard includes
#include <QtCore/QString>
#include <string>
#include <type_traits>

namespace kate {

template <typename OutputString, typename InputString>
typename std::enable_if<
    !(std::is_same<InputString, const char*>::value || std::is_same<InputString, clang::DCXString>::value)
  , OutputString
  >::type string_cast(const InputString&);

template <>
inline std::string string_cast<std::string, QString>(const QString& str)
{
#ifndef QT_NO_STL
    return str.toStdString();
#else
    return str.toUtf8().constData();
#endif
}

template<>
inline QString string_cast<QString, std::string>(const std::string& str)
{
#ifndef QT_NO_STL
    return QString::fromStdString(str);
#else
    return str.c_str();
#endif
}

template <typename OutputString, typename InputString>
typename std::enable_if<
    (std::is_same<OutputString, QString>::value || std::is_same<OutputString, std::string>::value)
  && std::is_same<InputString, const char*>::value
  , OutputString
  >::type string_cast(InputString c_str)
{
    return c_str ? c_str : OutputString{};
}

template <typename OutputString, typename InputString>
typename std::enable_if<
    (std::is_same<OutputString, QString>::value || std::is_same<OutputString, std::string>::value)
  && std::is_same<InputString, clang::DCXString>::value
  , OutputString
  >::type string_cast(const InputString& str)
{
    const auto* const c_str = clang_getCString(str);
    return c_str ? c_str : OutputString{};
}

}                                                           // namespace kate
