/**
 * \file
 *
 * \brief Class tester for \c indexer
 *
 * \date Wed Oct  9 11:24:19 MSK 2013 -- Initial design
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
#include <src/test/indexer_tester.h>
#include <config.h>

// Standard includes
#include <boost/filesystem/operations.hpp>
// Include the following file if u need to validate some text results
// #include <boost/test/output_test_stream.hpp>
#include <KDE/KDebug>
#include <QtTest/QtTest>
#include <chrono>
#include <iostream>
#include <vector>

// Uncomment if u want to use boost test output streams.
// Then just output smth to it and validate an output by
// BOOST_CHECK(out_stream.is_equal("Test text"))
// using boost::test_tools::output_test_stream;

using namespace kate;

namespace {

std::vector<const char*> PARSE_OPTIONS = {
    "-x", "c++"
  , "-std=c++11"
  , "-D__GXX_EXPERIMENTAL_CXX0X__"
  , "-I/usr/include"
  , "-I/usr/lib/gcc/x86_64-pc-linux-gnu/4.8.1/include/g++-v4"
  , "-I/usr/lib/gcc/x86_64-pc-linux-gnu/4.8.1/include/g++-v4/x86_64-pc-linux-gnu"
  , "-I/usr/lib/gcc/x86_64-pc-linux-gnu/4.8.1/include/g++-v4/backward"
  , "-I/usr/lib/gcc/x86_64-pc-linux-gnu/4.8.1/include"
  , "-I/usr/local/include"
  , "-I/usr/lib/gcc/x86_64-pc-linux-gnu/4.8.1/include-fixed"
  , "-I" CMAKE_SOURCE_DIR
  , "-I" CMAKE_SOURCE_DIR "/src/test/data"
};

const std::string SAMPLE_DB_PATH = CMAKE_BINARY_DIR "/src/test/data/test.db";
const char* const SAMPLE_DIR = CMAKE_SOURCE_DIR "/src/test/data";
const char* const VAR_TEST_09 = CMAKE_SOURCE_DIR "/src/test/data/variables/test_009.cc";
const index::dbid SAMPLE_ID = 0xdeadcafe;

bool make_sure_database_not_exists(const boost::filesystem::path& path)
{
    boost::system::error_code error;
    boost::filesystem::remove_all(path, error);
    return bool(!error);
}
bool s_db_rm_flag = make_sure_database_not_exists(SAMPLE_DB_PATH);
}                                                           // anonymous namespace

void result_waiter::finished()
{
    m_done = true;
    kDebug() << "--- Got it! ---";
}

indexer_tester::indexer_tester()
  : m_indexer{SAMPLE_ID, SAMPLE_DB_PATH}
{
    QVERIFY(s_db_rm_flag);
    connect(&m_indexer, SIGNAL(finished()), &m_res, SLOT(finished()));
}

void indexer_tester::index_sample_file()
{
    kDebug() << "DB path:" << SAMPLE_DB_PATH.c_str();
    m_indexer.set_compiler_options(decltype(PARSE_OPTIONS){PARSE_OPTIONS})
      .add_target(QString{VAR_TEST_09})
      .start()
      ;
    auto start_time = std::chrono::system_clock::now();
    auto stop_requested = false;
    while (!m_res.m_done)
    {
        QCoreApplication::processEvents();
        if (!stop_requested)
        {
            auto now = std::chrono::system_clock::now();
            if ((now - start_time) > std::chrono::seconds(2))
            {
                kDebug() << "Requesting indexer to stop!";
                m_indexer.stop();
                stop_requested = true;
            }
        }
    }
    kDebug() << "Done";
}

QTEST_MAIN(indexer_tester)
