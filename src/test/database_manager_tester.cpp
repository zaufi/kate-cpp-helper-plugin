/**
 * \file
 *
 * \brief Class tester for \c DatabaseManager
 *
 * \date Sun Oct 13 09:17:11 MSK 2013 -- Initial design
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
#include <src/database_manager.h>
#include <config.h>

// Standard includes
#include <boost/filesystem/operations.hpp>
#include <boost/test/auto_unit_test.hpp>
// Include the following file if u need to validate some text results
// #include <boost/test/output_test_stream.hpp>
#include <iostream>

// Uncomment if u want to use boost test output streams.
// Then just output smth to it and validate an output by
// BOOST_CHECK(out_stream.is_equal("Test text"))
// using boost::test_tools::output_test_stream;

using namespace kate;

namespace {
const char* const SAMPLE_DB_PATH_CSTR = CMAKE_BINARY_DIR "/src/test/data/";
const KUrl SAMPLE_DB_PATH = QString{SAMPLE_DB_PATH_CSTR};
}                                                           // anonymous namespace

BOOST_AUTO_TEST_CASE(database_manager_test)
{
    BOOST_REQUIRE(boost::filesystem::exists(SAMPLE_DB_PATH_CSTR));
    QStringList enabled_dbs;
    enabled_dbs << "test";
    DatabaseManager mgr(SAMPLE_DB_PATH, enabled_dbs);
}
