/**
 * \file
 *
 * \brief Class tester for \c sanitize_snippet
 *
 * \date Thu Dec 20 21:04:16 MSK 2012 -- Initial design
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
#include <src/sanitize_snippet.h>

// Standard includes
// ALERT The following #define must be enabled only in one translation unit
// per unit test binary (which may consists of several such modules)
// #define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/auto_unit_test.hpp>
// Include the following file if u need to validate some text results
// #include <boost/test/output_test_stream.hpp>
#include <KDebug>
#include <iostream>

// Uncomment if u want to use boost test output streams.
//  Then just output smth to it and valida an output by
//  BOOST_CHECK(out_stream.is_equal("Test text"))
// using boost::test_tools::output_test_stream;

namespace {
std::vector<std::pair<std::string, std::string>> INPUT_OUTPUT_LIST = {
    { "void *" , "void*" }
  , { "int &" , "int&" }
  , { "std::basic_string<char> &&" , "std::string&&" }
  , {
        "std::list<int, std::allocator<int> >"
      , "std::list<int>"
    }
  , {
        "std::vector<std::pair<int, long>, std::allocator<std::pair<int, long> > >"
      , "std::vector<std::pair<int, long>>"
    }
  , {
        "std::map<int, long, std::less<int>, std::allocator<std::pair<const int, long> > >"
      , "std::map<int, long>"
    }
  , {
        "std::map<"
            "std::pair<int, long>"
          ", std::basic_string<char>"
          ", std::less<std::pair<int, long> >"
          ", std::allocator<std::pair<const std::pair<int, long>, std::basic_string<char> > > "
          ">"
      , "std::map<std::pair<int, long>, std::string>"
    }
};
}                                                           // anonymous namespace

BOOST_AUTO_TEST_CASE(sanitize_snippet_test)
{
    for (const auto& p : INPUT_OUTPUT_LIST)
    {
        BOOST_TEST_MESSAGE("input: " << p.first);
        BOOST_TEST_MESSAGE("expected: " << p.second);
        QString r = kate::sanitizePrefix(p.first.c_str());
        BOOST_TEST_MESSAGE("result: " << r.toStdString());
        BOOST_CHECK_EQUAL(r.toStdString(), p.second);
    }
}

// kate: hl C++11/Qt4;
