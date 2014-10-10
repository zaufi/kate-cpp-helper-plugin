/**
 * \file
 *
 * \brief Class tester for \c index_utils
 *
 * \date Tue Oct 29 06:44:47 MSK 2013 -- Initial design
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
#include "../index/utils.h"

// Standard includes
#include <boost/test/auto_unit_test.hpp>
// Include the following file if u need to validate some text results
// #include <boost/test/output_test_stream.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <iostream>
#include <iomanip>
#include <byteswap.h>

// Uncomment if u want to use boost test output streams.
// Then just output smth to it and validate an output by
// BOOST_CHECK(out_stream.is_equal("Test text"))
// using boost::test_tools::output_test_stream;

namespace {
const std::string SAMPLE_UUID = "70b9b979-c3c7-4837-a58f-cb7e9461da39";
const boost::uuids::string_generator UUID_PARSER = {};
}                                                           // anonymous namespace

using namespace kate::index;

BOOST_AUTO_TEST_CASE(index_utils_test_1)
{
    auto uuid = UUID_PARSER(SAMPLE_UUID);
    std::cout << "uuid=" << uuid << std::endl;
    auto id = make_dbid(uuid);
    std::cout << "id=" << std::hex << id << std::endl;
    auto first = std::string{begin(SAMPLE_UUID), begin(SAMPLE_UUID) + 8};
    auto c = bswap_32(std::stoul(first, nullptr, 16));
    std::cout << "id2=" << std::hex << c << std::endl;
    BOOST_CHECK_EQUAL(c, id);
}
