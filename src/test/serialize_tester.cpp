/**
 * \file
 *
 * \brief Class tester for \c serialize
 *
 * \date Sun Oct 27 17:38:10 MSK 2013 -- Initial design
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
#include "../index/serialize.h"

// Standard includes
#include <boost/test/auto_unit_test.hpp>
// Include the following file if u need to validate some text results
// #include <boost/test/output_test_stream.hpp>
#include <iostream>

// Uncomment if u want to use boost test output streams.
// Then just output smth to it and validate an output by
// BOOST_CHECK(out_stream.is_equal("Test text"))
// using boost::test_tools::output_test_stream;


using namespace kate::index;

BOOST_AUTO_TEST_CASE(serialize_test)
{
    {
        auto a = 0xdeadc0de;
        auto str = serialize(a);
        BOOST_CHECK_EQUAL(str.size(), sizeof(a));
        auto b = deserialize<decltype(a)>(str);
        BOOST_CHECK_EQUAL(a, b);
    }
    {
        auto a = 0xdeadc0defacecafe;
        auto str = serialize(a);
        BOOST_CHECK_EQUAL(str.size(), sizeof(a));
        auto b = deserialize<decltype(a)>(str);
        BOOST_CHECK_EQUAL(a, b);
    }
}

