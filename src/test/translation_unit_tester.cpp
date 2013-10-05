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
#include <src/clang_utils.h>
#include <config.h>

// Standard includes
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

    DCXIndex m_index = {clang_createIndex(0, 1)};
    QStringList m_options;
};

struct indexer_data
{
    QString m_main_file;
};

const char* getCXIndexContainer(const CXIdxContainerInfo *info)
{
    CXIdxClientContainer container;
    container = clang_index_getClientContainer(info);
    if (!container)
        return "[<<NULL>>]";
    return (const char *)container;
}

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

BOOST_FIXTURE_TEST_CASE(translation_unit_test_1, fixture)
{
#if 0
    TranslationUnit unit = {
        m_index
      , KUrl{CMAKE_SOURCE_DIR "/src/test/data/sample.cpp"}
      , m_options
      , TranslationUnit::defaultEditingParseOptions()
      , TranslationUnit::unsaved_files_list_type()
    };
#endif
    IndexerCallbacks index_callbacks = {
        // abort query
        [](CXClientData client_data, void*) -> int
        {
            auto* const data = static_cast<indexer_data*>(client_data);
            kDebug(DEBUG_AREA) << "CB: abort query";
            return 0;
        }
      , [](CXClientData, CXDiagnosticSet, void*)
        {
            kDebug(DEBUG_AREA) << "CB: diagnostic";
        }
      , // entered main file
        [](CXClientData client_data, CXFile file, void*) -> CXIdxClientFile
        {
            kDebug(DEBUG_AREA) << "CB: entering" << file;
            auto* const data = static_cast<indexer_data*>(client_data);
            data->m_main_file = toString(file);
            return static_cast<CXIdxClientFile>(file);
        }
      , [](CXClientData client_data, const CXIdxIncludedFileInfo* info) -> CXIdxClientFile
        {
            kDebug(DEBUG_AREA) << "CB: #included file:" << info->filename
              << "isAngled:" << info->isAngled
              << "isImport:" << info->isImport
              << "isMod:" << info->isModuleImport
              ;
            return static_cast<CXIdxClientFile>(info->file);
        }
      , [](CXClientData client_data, const CXIdxImportedASTFileInfo* info) -> CXIdxClientASTFile
        {
            kDebug(DEBUG_AREA) << "CB: AST file imported" << info->file << ", impl: " << info->isImplicit;
            return static_cast<CXIdxClientFile>(info->file);
        }
      , [](CXClientData client_data, void*) -> CXIdxClientContainer
        {
            kDebug(DEBUG_AREA) << "CB: TU started";
            return nullptr;
        }
      , [](CXClientData client_data, const CXIdxDeclInfo* info)
        {
            const char* name = info->entityInfo->name ? info->entityInfo->name : "anonymous";
            kDebug(DEBUG_AREA) << "CB: index declaration: name =" << name;
            kDebug(DEBUG_AREA) << "CB: index declaration: kind =" << toString(info->entityInfo->kind) <<
              ' ' << toString(info->entityInfo->templateKind);
            kDebug(DEBUG_AREA) << "CB: index declaration: cursor:" << info->cursor;
            kDebug(DEBUG_AREA) << "CB: index declaration: semantic container:" << getCXIndexContainer(info->semanticContainer);
            kDebug(DEBUG_AREA) << "CB: index declaration: lexican container:" << getCXIndexContainer(info->lexicalContainer);
        }
      , [](CXClientData client_data, const CXIdxEntityRefInfo* info)
        {
            kDebug(DEBUG_AREA) << "CB: index reference";
        }
    };

    indexer_data data;
    std::vector<QByteArray> utf8_options;
    std::vector<const char*> clang_options;
    TranslationUnit::transform_command_line_args(m_options, utf8_options, clang_options);
    DCXIndexAction action = {clang_IndexAction_create(m_index)};
    auto result = clang_indexSourceFile(
        action
      , &data
      , &index_callbacks
      , sizeof(index_callbacks)
        // CXIndexOpt_SuppressRedundantRefs
      , CXIndexOpt_IndexFunctionLocalSymbols | CXIndexOpt_SkipParsedBodiesInSession | CXIndexOpt_SuppressRedundantRefs
      , CMAKE_SOURCE_DIR "/src/test/data/sample.cpp"
      , clang_options.data()
      , clang_options.size()
      , nullptr
      , 0
      , nullptr
      , clang_defaultEditingTranslationUnitOptions()
      );
}
