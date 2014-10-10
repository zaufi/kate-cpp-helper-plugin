/**
 * \file
 *
 * \brief Sample indexer
 *
 * \date Sun Oct  6 06:27:22 MSK 2013 -- Initial design
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
#include "../clang/disposable.h"
#include "../clang/kind_of.h"
#include "../clang/location.h"
#include "../clang/to_string.h"
#include <config.h>

// Standard includes
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

using namespace kate;
using namespace kate::clang;

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

constexpr std::size_t MAX_BUFFER = 8192;

void print_file(const char* const file)
{
    std::ifstream in(file);
    std::cout << "---[" << file << "]---\n";
    for (auto i = 1; !in.eof(); ++i)
    {
        std::string line;
        std::getline(in, line);
        std::cout << std::setw(3) << i << ": " << line << std::endl;
    }
    std::cout << "---[EOF]---\n";
}

struct indexer_data
{
    std::string m_main_file;
    std::vector<std::unique_ptr<std::string>> m_containers;

    std::ostream& out()
    {
        if (!m_main_file.empty())
            std::cout << m_main_file << ": ";
        return std::cout;
    }

    std::ostream& out(CXFile file)
    {
        assert(file);
        auto filename = to_string(file);
        std::cout << filename << ": ";
        return std::cout;
    }

    void out_location(CXIdxLoc l)
    {
        location loc{l};
        std::cout << loc.file().toLocalFile().toUtf8().constData()
          << ':' << loc.line()
          << ':' << loc.column()
          << ' '
          ;
    }

    void out_cursor(const CXCursor& cursor)
    {
        std::cout
          << "  cursor: " << location(cursor) << std::endl
          << "    kind: " << to_string(kind_of(cursor)) << std::endl
          << "    link: " << to_string(clang_getCursorLinkage(cursor)) << std::endl
          ;
    }

    void out_entity_info(const char* const cb, const CXIdxEntityInfo* info)
    {
        auto kind = kind_of(*info);
        std::cout << cb << ": '" << (info->name ? info->name : "<null>")
          << "', kind: " << to_string(kind)
          << (may_apply_template_kind(kind) ? to_string(info->templateKind) : std::string())
          << " [" << info->templateKind << "]" << std::endl
#if 0
          << "     USR: " << (info->USR ? info->USR : "<no-USR>")
#endif
          << std::endl
          ;
        out_cursor(info->cursor);
        //
        auto type = clang_getCursorType(info->cursor);
        std::cout << "    type: " << to_string(DCXString{clang_getTypeSpelling(type)}) << std::endl;
        /// \todo Show attributes
    }

    void out_base_class(const int n, const CXIdxEntityInfo* info)
    {
        auto kind = kind_of(*info);
        std::cout << "     base " << n << ": " << to_string(kind) << ' '
          << (info->name ? info->name : "<null>")
          << std::endl;
    }

    void out_container(const char* const cntr, const CXIdxContainerInfo* info)
    {
        CXIdxClientContainer container = clang_index_getClientContainer(info);
        std::cout << cntr << ".cntr: ";
        if (!container)
            std::cout << "<NULL>";
        else
            std::cout << *(const std::string*)container;
        std::cout << std::endl;
    }

    CXIdxClientContainer updateClientContainer(const CXIdxEntityInfo* info, const CXIdxLoc l)
    {
        std::string container;
        if (info->name)
            container = info->name;
        else
            container = "<anonymous-tag>";
        //
        location loc = {l};
        auto result = std::string{loc.file().toLocalFile().toUtf8().constData()} + ":"
          + std::to_string(loc.line()) + ":"
          + std::to_string(loc.column()) + ": "
          + container
          ;
        auto& ptr = *m_containers.emplace(end(m_containers), new std::string(std::move(result)));
        return CXIdxClientContainer(ptr.get());
    }
};

int on_abort_cb(CXClientData client_data, void*)
{
    auto* const data = static_cast<indexer_data*>(client_data);
    data->out() << "Abort query" << std::endl;
    return 0;
}

void on_diagnostic_cb(CXClientData, CXDiagnosticSet, void*)
{
    std::cout << "CB: diagnostic" << std::endl;
}

CXIdxClientFile on_entering_main_file(CXClientData client_data, CXFile file, void*)
{
    auto* const data = static_cast<indexer_data*>(client_data);
    data->m_main_file = to_string(file);
    data->out() << "Entering main file" << std::endl;
    return static_cast<CXIdxClientFile>(file);
}

CXIdxClientFile on_include_file(CXClientData client_data, const CXIdxIncludedFileInfo* info)
{
    auto* const data = static_cast<indexer_data*>(client_data);
    char open = info->isAngled ? '<' : '"';
    char close = info->isAngled ? '>' : '"';
    data->out(info->file) << "#include "
        << open << info->filename << close
        << " at line " << location(info->hashLoc).line()
        << std::endl;
    return static_cast<CXIdxClientFile>(info->file);
}

CXIdxClientASTFile on_include_ast_file(CXClientData client_data, const CXIdxImportedASTFileInfo* info)
{
    auto* const data = static_cast<indexer_data*>(client_data);
    data->out(info->file) << "Importing AST file at line "
      << location(info->loc).line() << std::endl;
    return static_cast<CXIdxClientFile>(info->file);
}

CXIdxClientContainer on_translation_unit(CXClientData client_data, void*)
{
    auto* const data = static_cast<indexer_data*>(client_data);
    data->out() << "Translation unit has started" << std::endl;
    assert(data->m_containers.empty());
    data->m_containers.emplace_back(new std::string("TU"));
    /// \todo ALERT DIRTY HACK
    return CXIdxClientContainer(data->m_containers.begin()->get());
}

void on_declaration(CXClientData client_data, const CXIdxDeclInfo* info)
{
    std::cout << "---------------------------------------------" << std::endl;
    auto* const data = static_cast<indexer_data*>(client_data);
    data->out_location(info->loc);
    data->out_entity_info("decl", info->entityInfo);
    std::cout << "    misc:";
    if (info->isDefinition)
        std::cout << " definition";
    if (info->isRedeclaration)
        std::cout << " redeclaration";
    if (info->isContainer)
        std::cout << " container";
    if (info->flags & CXIdxDeclFlag_Skipped)
        std::cout << " skipped";
    if (info->isImplicit)
        std::cout << " implicit";
    if (info->numAttributes)
        std::cout << info->numAttributes << " attrs";
    std::cout << std::endl;
    //
    const auto* class_info = clang_index_getCXXClassDeclInfo(info);
    if (class_info)
    {
        for (unsigned i = 0; i != class_info->numBases; ++i)
        {
            data->out_base_class(i, class_info->bases[i]->base);
        }
    }
    data->out_container("sem", info->semanticContainer);
    data->out_container("lex", info->lexicalContainer);

    auto brief = to_string(DCXString{clang_Cursor_getBriefCommentText(info->cursor)});
    if (!brief.empty())
    {
        std::cout << "comment: '" << brief << "'" << std::endl;
    }

    if (info->declAsContainer)
        clang_index_setClientContainer(
            info->declAsContainer
          , data->updateClientContainer(info->entityInfo, info->loc)
          );
}

void on_declaration_reference(CXClientData client_data, const CXIdxEntityRefInfo* info)
{
    std::cout << "---------------------------------------------" << std::endl;
    auto* const data = static_cast<indexer_data*>(client_data);
    data->out_location(info->loc);
    data->out_entity_info("ref", info->referencedEntity);
    data->out_entity_info("parent", info->parentEntity);
#if 0
    clang_getCursorReferenceNameRange(info->cursor);
#endif
}


void index_source_file(const char* const file)
{
    DCXIndex index = {clang_createIndex(1, 1)};
    DCXIndexAction action = {clang_IndexAction_create(index)};
    IndexerCallbacks index_callbacks = {
        &on_abort_cb
      , &on_diagnostic_cb
      , &on_entering_main_file
      , &on_include_file
      , &on_include_ast_file
      , &on_translation_unit
      , &on_declaration
      , &on_declaration_reference
    };
    indexer_data data;
    auto result = clang_indexSourceFile(
        action
      , &data
      , &index_callbacks
      , sizeof(index_callbacks)
        // CXIndexOpt_SuppressRedundantRefs
        // CXIndexOpt_SkipParsedBodiesInSession
      , CXIndexOpt_IndexFunctionLocalSymbols
      , file
      , PARSE_OPTIONS.data()
      , PARSE_OPTIONS.size()
      , nullptr
      , 0
      , nullptr
      , clang_defaultEditingTranslationUnitOptions()
      );

}

int main(int argc, const char* const argv[])
{
    if (argc < 2)
    {
        std::cerr << "*** Error: No file given" << std::endl;
        return EXIT_FAILURE;
    }


    for (int i = 1; i < argc; ++i)
    {
        print_file(argv[i]);
        index_source_file(argv[i]);
        std::cout << std::endl;
    }

    return EXIT_SUCCESS;
}
