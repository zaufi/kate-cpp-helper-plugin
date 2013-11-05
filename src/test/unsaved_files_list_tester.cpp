/**
 * \file
 *
 * \brief Class tester for \c unsaved_files_list
 *
 * \date Sun Nov  3 00:00:38 MSK 2013 -- Initial design
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
#include <src/clang/unsaved_files_list.h>

// Standard includes
#include <boost/algorithm/string.hpp>
#include <boost/test/auto_unit_test.hpp>
// Include the following file if u need to validate some text results
// #include <boost/test/output_test_stream.hpp>
#include <iostream>

// Uncomment if u want to use boost test output streams.
// Then just output smth to it and validate an output by
// BOOST_CHECK(out_stream.is_equal("Test text"))
// using boost::test_tools::output_test_stream;

using namespace kate::clang;

BOOST_AUTO_TEST_CASE(unsaved_files_list_test)
{
    unsaved_files_list l;
    {
        auto f = l.get();
        BOOST_CHECK(f.empty());
    }
    l.update(KUrl{"/test1"}, "test1 content");
    l.finalize_updating();
    {
        auto f = l.get();
        BOOST_CHECK_EQUAL(f.size(), 1u);
        BOOST_CHECK((boost::equals(f[0].Filename, "/test1")));
        BOOST_CHECK((boost::equals(f[0].Contents, "test1 content")));
    }
    l.update(KUrl{"/test2"}, "");
    l.finalize_updating();
    {
        auto f = l.get();
        BOOST_CHECK_EQUAL(f.size(), 1u);
        BOOST_CHECK((boost::equals(f[0].Filename, "/test2")));
        BOOST_CHECK((boost::equals(f[0].Contents, "")));
    }
    l.update(KUrl{"/test1"}, "test1 content");
    l.update(KUrl{"/test2"}, "");
    l.finalize_updating();
    {
        auto f = l.get();
        BOOST_CHECK_EQUAL(f.size(), 2u);
    }
    l.update(KUrl{"/test2"}, "test2 content");
    l.finalize_updating();
    {
        auto f = l.get();
        BOOST_CHECK_EQUAL(f.size(), 1u);
        BOOST_CHECK((boost::equals(f[0].Filename, "/test2")));
        BOOST_CHECK((boost::equals(f[0].Contents, "test2 content")));
    }
}

BOOST_AUTO_TEST_CASE(unsaved_files_list_double_update_test)
{
    unsaved_files_list l;
    l.update(KUrl{"/test1"}, "test1 content");
    l.update(KUrl{"/test1"}, "test2 content");
    {
        auto f = l.get();
        BOOST_CHECK_EQUAL(f.size(), 1u);
        BOOST_CHECK((boost::equals(f[0].Filename, "/test1")));
        BOOST_CHECK((boost::equals(f[0].Contents, "test2 content")));
    }
}
