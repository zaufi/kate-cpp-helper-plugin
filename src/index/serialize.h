/**
 * \file
 *
 * \brief Simple, straightforward, naive and limited serialization implementation
 *
 * \date Sat Oct 26 13:38:50 MSK 2013 -- Initial design
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
#include <algorithm>
#include <cassert>
#include <string>
#include <type_traits>
#include <utility>

namespace kate { namespace index {

/// Simple serialization helper for integral types
template <typename T>
typename std::enable_if<
    std::is_integral<T>::value
  , std::string
  >::type serialize(const T value)
{
    union
    {
        T as_int;
        char buf[sizeof(T)];
    };
    as_int = value;
    auto result = std::string{sizeof(as_int), char{}};
    std::copy(buf, buf + sizeof(T), begin(result));
    return result;
}

/// Simple deserialization helper for integral types
template <typename T>
typename std::enable_if<
    std::is_integral<T>::value
  , T
  >::type deserialize(const std::string& raw)
{
    assert("Size mismatch" && sizeof(T) <= raw.size());
    union
    {
        T as_int;
        char buf[sizeof(T)];
    };
    std::copy(begin(raw), end(raw), buf);
    return as_int;
}

}}                                                          // namespace index, kate
