/**
 * \file
 *
 * \brief Class tester for \c translation_unit
 *
 * \date Thu Oct  3 22:01:37 MSK 2013 -- Initial design
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
#include <src/translation_unit.h>
#include <src/clang/disposable.h>
#include <config.h>

// Standard includes
#include <boost/test/auto_unit_test.hpp>
// Include the following file if u need to validate some text results
// #include <boost/test/output_test_stream.hpp>
#include <iostream>
#include <fstream>

// Uncomment if u want to use boost test output streams.
// Then just output smth to it and validate an output by
// BOOST_CHECK(out_stream.is_equal("Test text"))
// using boost::test_tools::output_test_stream;

using namespace kate;

namespace {
struct fixture
{
    fixture()
    {
        m_options << "-x" << "c++"
          << "-std=c++11" << "-D__GXX_EXPERIMENTAL_CXX0X__"
          << "-I/usr/include"
          << "-I/usr/lib/gcc/x86_64-pc-linux-gnu/4.8.1/include/g++-v4"
          << "-I/usr/lib/gcc/x86_64-pc-linux-gnu/4.8.1/include/g++-v4/x86_64-pc-linux-gnu"
          << "-I/usr/lib/gcc/x86_64-pc-linux-gnu/4.8.1/include/g++-v4/backward"
          << "-I/usr/lib/gcc/x86_64-pc-linux-gnu/4.8.1/include"
          << "-I/usr/local/include"
          << "-I/usr/lib/gcc/x86_64-pc-linux-gnu/4.8.1/include-fixed"
          << "-I" CMAKE_SOURCE_DIR
          << "-I" CMAKE_SOURCE_DIR "/src/test/data"
          ;
    }

    clang::DCXIndex m_index = {clang_createIndex(0, 1)};
    QStringList m_options;
};

}                                                           // anonymous namespace

BOOST_FIXTURE_TEST_CASE(translation_unit_test_0, fixture)
{
    try
    {
        TranslationUnit unit = {
            m_index
          , KUrl{CMAKE_SOURCE_DIR "/src/test/data/not-existed.cpp"}
          , m_options
          , TranslationUnit::defaultEditingParseOptions()
          , TranslationUnit::unsaved_files_list_type()
        };
        BOOST_FAIL("Unexpected success!");
    }
    catch (TranslationUnit::Exception::ParseFailure&)
    {
        BOOST_TEST_PASSPOINT();
    }
}
