/**
 * \file
 *
 * \brief Class \c kate::sample (implementation)
 *
 * \date Thu Oct  3 23:19:28 MSK 2013 -- Initial design
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
#if 0
#include <src/test/data/sample.h>
#include <src/test/data/other.h>
#endif

// Standard includes
// #include <iostream>

#if 0
template <typename... Args>
long bar(Args... args)
{
    long sz = sizeof...(args);
    return sz;
}
#endif

namespace details {
int foo() {return 0;}
}                                                           // namespace details

namespace anonymous {
struct zaq
{
    zaq() { }
    ~zaq() { }
    zaq(const zaq&) { }
    zaq(zaq&&) { }
    zaq& operator=(const zaq& zzz) { return *this; }
    zaq& operator=(zaq&&);
};

inline zaq& zaq::operator=(zaq&& qqq)
{
#if 0
    std::cout << __PRETTY_FUNCTION__ << std::endl;
#endif
    return *this;
}

struct qaz : public zaq {};
}                                                           // anonymous namespace

long a = 123;

int main()
{
#if 0
    std::cout << __PRETTY_FUNCTION__ << std::endl;
#endif
    return bar(1, 'a', nullptr) + details::foo();
}
