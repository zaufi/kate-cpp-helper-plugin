/**
 * \file
 *
 * \brief Class tester for \c HeaderFilesCache
 *
 * \date Sat Dec 29 08:00:37 MSK 2012 -- Initial design
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

// Project specific includes
#include <src/header_files_cache.h>

// Standard includes
#include <boost/test/auto_unit_test.hpp>
// Include the following file if u need to validate some text results
// #include <boost/test/output_test_stream.hpp>
#include <iostream>

// Uncomment if u want to use boost test output streams.
//  Then just output smth to it and validate an output by
//  BOOST_CHECK(out_stream.is_equal("Test text"))
// using boost::test_tools::output_test_stream;

using kate::HeaderFilesCache;

// Your first test function :)
BOOST_AUTO_TEST_CASE(HeaderFilesCacheTest)
{
    HeaderFilesCache c;
    BOOST_CHECK_EQUAL(c.isEmpty(), true);
    BOOST_CHECK_EQUAL(c.size(), 0u);

    {
        const HeaderFilesCache& cache = c;
        BOOST_CHECK_EQUAL(cache["/some/file/path"], HeaderFilesCache::NOT_FOUND);
    }

    const int ID = c["/some/file/path"];
    const int SAME_ID = c["/some/file/path"];

    BOOST_CHECK_EQUAL(c.isEmpty(), false);
    BOOST_CHECK_EQUAL(c.size(), 1u);

    BOOST_CHECK_EQUAL(ID, SAME_ID);
    {
        const HeaderFilesCache& cache = c;
        BOOST_CHECK_EQUAL(cache["/some/file/path"], ID);
        BOOST_CHECK_EQUAL(cache["/some/file/path"], SAME_ID);
    }

    const int ID2 = c["/some/file/path2"];
    const int SAME_ID2 = c["/some/file/path2"];

    BOOST_CHECK_EQUAL(c.isEmpty(), false);
    BOOST_CHECK_EQUAL(c.size(), 2u);

    BOOST_CHECK_NE(ID, ID2);
    BOOST_CHECK_EQUAL(ID2, SAME_ID2);
    {
        const HeaderFilesCache& cache = c;
        BOOST_CHECK_EQUAL(cache["/some/file/path2"], ID2);
        BOOST_CHECK_EQUAL(cache["/some/file/path2"], SAME_ID2);
    }
}
