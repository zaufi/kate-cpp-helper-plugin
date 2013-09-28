/**
 * \file
 *
 * \brief Tests for utility functions
 *
 * \date Mon Feb  6 04:00:24 MSK 2012 -- Initial design
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
#include <src/utils.h>

// Standard includes
#include <boost/test/auto_unit_test.hpp>
// Include the following file if u need to validate some text results
// #include <boost/test/output_test_stream.hpp>
#include <iostream>

// Uncomment if u want to use boost test output streams.
//  Then just output smth to it and validate an output by
//  BOOST_CHECK(out_stream.is_equal("Test text"))
// using boost::test_tools::output_test_stream;

using kate::parseIncludeDirective;

namespace {
void ok_parser_test(const bool f)
{
    BOOST_TEST_MESSAGE("Testing parser w/ strict flag: " << f);
    {
        kate::IncludeParseResult r = parseIncludeDirective("#include <foo.h>", f);
        BOOST_CHECK_EQUAL(r.m_range.start().column(), 10);
        BOOST_CHECK_EQUAL(r.m_range.end().column(), 15);
        BOOST_CHECK_EQUAL(r.m_is_complete, true);
    }
    {
        kate::IncludeParseResult r = parseIncludeDirective("#  include <foo.h>", f);
        BOOST_CHECK_EQUAL(r.m_range.start().column(), 12);
        BOOST_CHECK_EQUAL(r.m_range.end().column(), 17);
        BOOST_CHECK_EQUAL(r.m_is_complete, true);
    }
    {
        kate::IncludeParseResult r = parseIncludeDirective("  #  include <foo.h>", f);
        BOOST_CHECK_EQUAL(r.m_range.start().column(), 14);
        BOOST_CHECK_EQUAL(r.m_range.end().column(), 19);
        BOOST_CHECK_EQUAL(r.m_is_complete, true);
    }
    {
        kate::IncludeParseResult r = parseIncludeDirective("  #include <foo.h>", f);
        BOOST_CHECK_EQUAL(r.m_range.start().column(), 12);
        BOOST_CHECK_EQUAL(r.m_range.end().column(), 17);
        BOOST_CHECK_EQUAL(r.m_is_complete, true);
    }
    {
        kate::IncludeParseResult r = parseIncludeDirective("#include <foo.h>  ", f);
        BOOST_CHECK_EQUAL(r.m_range.start().column(), 10);
        BOOST_CHECK_EQUAL(r.m_range.end().column(), 15);
        BOOST_CHECK_EQUAL(r.m_is_complete, true);
    }
    {
        kate::IncludeParseResult r = parseIncludeDirective("#include <foo.h>  // comment", f);
        BOOST_CHECK_EQUAL(r.m_range.start().column(), 10);
        BOOST_CHECK_EQUAL(r.m_range.end().column(), 15);
        BOOST_CHECK_EQUAL(r.m_is_complete, true);
    }

    {
        kate::IncludeParseResult r = parseIncludeDirective("#include <", f);
        if (f)
        {
            BOOST_CHECK_EQUAL(r.m_range.start().column(), -1);
            BOOST_CHECK_EQUAL(r.m_range.end().column(), -1);
        }
        else
        {
            BOOST_CHECK_EQUAL(r.m_range.start().column(), 10);
            BOOST_CHECK_EQUAL(r.m_range.end().column(), 10);
        }
        BOOST_CHECK_EQUAL(r.m_is_complete, false);
    }
    {
        kate::IncludeParseResult r = parseIncludeDirective("#include <f", f);
        if (f)
        {
            BOOST_CHECK_EQUAL(r.m_range.start().column(), -1);
            BOOST_CHECK_EQUAL(r.m_range.end().column(), -1);
        }
        else
        {
            BOOST_CHECK_EQUAL(r.m_range.start().column(), 10);
            BOOST_CHECK_EQUAL(r.m_range.end().column(), 11);
        }
        BOOST_CHECK_EQUAL(r.m_is_complete, false);
    }
}

void fail_parser_test(const bool f)
{
    {
        kate::IncludeParseResult r = parseIncludeDirective("", f);
        BOOST_CHECK_EQUAL(r.m_range.start().column(), -1);
        BOOST_CHECK_EQUAL(r.m_range.end().column(), -1);
        BOOST_CHECK_EQUAL(r.m_is_complete, false);
    }
    {
        kate::IncludeParseResult r = parseIncludeDirective("#", f);
        BOOST_CHECK_EQUAL(r.m_range.start().column(), -1);
        BOOST_CHECK_EQUAL(r.m_range.end().column(), -1);
        BOOST_CHECK_EQUAL(r.m_is_complete, false);
    }
    {
        kate::IncludeParseResult r = parseIncludeDirective("  ", f);
        BOOST_CHECK_EQUAL(r.m_range.start().column(), -1);
        BOOST_CHECK_EQUAL(r.m_range.end().column(), -1);
        BOOST_CHECK_EQUAL(r.m_is_complete, false);
    }
    {
        kate::IncludeParseResult r = parseIncludeDirective(" #", f);
        BOOST_CHECK_EQUAL(r.m_range.start().column(), -1);
        BOOST_CHECK_EQUAL(r.m_range.end().column(), -1);
        BOOST_CHECK_EQUAL(r.m_is_complete, false);
    }
    {
        kate::IncludeParseResult r = parseIncludeDirective(" # ", f);
        BOOST_CHECK_EQUAL(r.m_range.start().column(), -1);
        BOOST_CHECK_EQUAL(r.m_range.end().column(), -1);
        BOOST_CHECK_EQUAL(r.m_is_complete, false);
    }
    {
        kate::IncludeParseResult r = parseIncludeDirective("# ", f);
        BOOST_CHECK_EQUAL(r.m_range.start().column(), -1);
        BOOST_CHECK_EQUAL(r.m_range.end().column(), -1);
        BOOST_CHECK_EQUAL(r.m_is_complete, false);
    }
    {
        kate::IncludeParseResult r = parseIncludeDirective("#s", f);
        BOOST_CHECK_EQUAL(r.m_range.start().column(), -1);
        BOOST_CHECK_EQUAL(r.m_range.end().column(), -1);
        BOOST_CHECK_EQUAL(r.m_is_complete, false);
    }
    {
        kate::IncludeParseResult r = parseIncludeDirective("#smth", f);
        BOOST_CHECK_EQUAL(r.m_range.start().column(), -1);
        BOOST_CHECK_EQUAL(r.m_range.end().column(), -1);
        BOOST_CHECK_EQUAL(r.m_is_complete, false);
    }
    {
        kate::IncludeParseResult r = parseIncludeDirective("# s", f);
        BOOST_CHECK_EQUAL(r.m_range.start().column(), -1);
        BOOST_CHECK_EQUAL(r.m_range.end().column(), -1);
        BOOST_CHECK_EQUAL(r.m_is_complete, false);
    }
    {
        kate::IncludeParseResult r = parseIncludeDirective("# smth", f);
        BOOST_CHECK_EQUAL(r.m_range.start().column(), -1);
        BOOST_CHECK_EQUAL(r.m_range.end().column(), -1);
        BOOST_CHECK_EQUAL(r.m_is_complete, false);
    }
    {
        kate::IncludeParseResult r = parseIncludeDirective("#include", f);
        BOOST_CHECK_EQUAL(r.m_range.start().column(), -1);
        BOOST_CHECK_EQUAL(r.m_range.end().column(), -1);
        BOOST_CHECK_EQUAL(r.m_is_complete, false);
    }
    {
        kate::IncludeParseResult r = parseIncludeDirective("#incude ", f);
        BOOST_CHECK_EQUAL(r.m_range.start().column(), -1);
        BOOST_CHECK_EQUAL(r.m_range.end().column(), -1);
        BOOST_CHECK_EQUAL(r.m_is_complete, false);
    }
    {
        kate::IncludeParseResult r = parseIncludeDirective("#incuded", f);
        BOOST_CHECK_EQUAL(r.m_range.start().column(), -1);
        BOOST_CHECK_EQUAL(r.m_range.end().column(), -1);
        BOOST_CHECK_EQUAL(r.m_is_complete, false);
    }
    {
        kate::IncludeParseResult r = parseIncludeDirective("#include smth", f);
        BOOST_CHECK_EQUAL(r.m_range.start().column(), -1);
        BOOST_CHECK_EQUAL(r.m_range.end().column(), -1);
        BOOST_CHECK_EQUAL(r.m_is_complete, false);
    }
    {
        kate::IncludeParseResult r = parseIncludeDirective("#include<", f);
        BOOST_CHECK_EQUAL(r.m_range.start().column(), -1);
        BOOST_CHECK_EQUAL(r.m_range.end().column(), -1);
        BOOST_CHECK_EQUAL(r.m_is_complete, false);
    }
}
}                                                           // anonymous namespace

BOOST_AUTO_TEST_CASE(parse_ok_test)
{
    ok_parser_test(false);
    ok_parser_test(true);
}

BOOST_AUTO_TEST_CASE(parse_failures_test)
{
    fail_parser_test(false);
    fail_parser_test(true);
}

BOOST_AUTO_TEST_CASE(partial_parse_test)
{
    BOOST_CHECK(kate::tryToCompleteIncludeDirective("") == QString());
    BOOST_CHECK(kate::tryToCompleteIncludeDirective("not #include at all") == QString());
    BOOST_CHECK(kate::tryToCompleteIncludeDirective("#") == QString());
    BOOST_CHECK(kate::tryToCompleteIncludeDirective("#i") == QString());
    BOOST_CHECK(kate::tryToCompleteIncludeDirective("#ia") == QString());

#if 0
    auto s = kate::tryToCompleteIncludeDirective("#in").toStdString();
    std::cout << s << std::endl;
#endif

    BOOST_CHECK(kate::tryToCompleteIncludeDirective("#in") == QString("#include"));
    BOOST_CHECK(kate::tryToCompleteIncludeDirective("#inc") == QString("#include"));
    BOOST_CHECK(kate::tryToCompleteIncludeDirective("#incl") == QString("#include"));
    BOOST_CHECK(kate::tryToCompleteIncludeDirective("#inclu") == QString("#include"));
    BOOST_CHECK(kate::tryToCompleteIncludeDirective("#includ") == QString("#include"));
    BOOST_CHECK(kate::tryToCompleteIncludeDirective("#include") == QString("#include"));

    BOOST_CHECK(kate::tryToCompleteIncludeDirective("#include_") == QString());

    BOOST_CHECK(kate::tryToCompleteIncludeDirective("# in") == QString("# include"));
    BOOST_CHECK(kate::tryToCompleteIncludeDirective("#  in") == QString("#  include"));
    BOOST_CHECK(kate::tryToCompleteIncludeDirective("#   in") == QString("#   include"));
}

BOOST_AUTO_TEST_CASE(is_suitable_document_test)
{
    using kate::isSuitableDocument;

    BOOST_CHECK_EQUAL(isSuitableDocument("application/xslt+xml", "xslt"), false);

    BOOST_CHECK_EQUAL(isSuitableDocument("text/x-c++hdr", "C++"), true);
    BOOST_CHECK_EQUAL(isSuitableDocument("text/x-c++hdr", "C++11"), true);
    BOOST_CHECK_EQUAL(isSuitableDocument("text/x-c++hdr", "C++11/Qt4"), true);
}

// kate: hl C++11/Qt4;
