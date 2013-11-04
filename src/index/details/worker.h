/**
 * \file
 *
 * \brief Class \c kate::index::details::worker (interface)
 *
 * \date Sun Nov  3 19:00:29 MSK 2013 -- Initial design
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

#pragma once

// Project specific includes
#include <src/clang/location.h>
#include <src/index/details/container_info.h>
#include <src/index/search_result.h>

// Standard includes
#include <clang-c/Index.h>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <atomic>
#include <map>
#include <memory>
#include <vector>

// fwd decls
class QFileInfo;
class KUrl;

namespace kate { namespace index {

// fwd decls
class document;
class indexer;

namespace details {

/**
 * \brief Worker class to do an indexer's job
 *
 * \internal Only \c kate::index::indexer can create instances of this class.
 *
 */
class worker : public QObject
{
    Q_OBJECT

public:
    explicit worker(indexer*);
    ~worker();

    bool is_cancelled() const;

public Q_SLOTS:
    void process();
    void request_cancel();

Q_SIGNALS:
    void indexing_uri(QString);
    void error(clang::location, QString);
    void finished();

private:
    struct declaration_location
    {
        fileid m_file_id;
        int m_line;
        int m_column;
        friend bool operator<(
            const declaration_location& lhs
          , const declaration_location& rhs
          )
        {
            return lhs.m_file_id < rhs.m_file_id
              || (lhs.m_file_id == rhs.m_file_id && lhs.m_line < rhs.m_line)
              || (lhs.m_file_id == rhs.m_file_id && lhs.m_line == rhs.m_line && lhs.m_column < rhs.m_column)
              ;
        }
    };

    bool dispatch_target(const KUrl&);
    bool dispatch_target(const QFileInfo&);
    void handle_file(const QString&);
    void handle_directory(const QString&);
    bool is_look_like_cpp_source(const QFileInfo&);

    template <typename... ClientArgs>
    CXIdxClientContainer update_client_container(ClientArgs&&...);

    static int on_abort_cb(CXClientData, void*);
    static void on_diagnostic_cb(CXClientData, CXDiagnosticSet, void*);
    static CXIdxClientFile on_entering_main_file(CXClientData, CXFile, void*);
    static CXIdxClientFile on_include_file(CXClientData, const CXIdxIncludedFileInfo*);
    static CXIdxClientASTFile on_include_ast_file(CXClientData, const CXIdxImportedASTFileInfo*);
    static CXIdxClientContainer on_translation_unit(CXClientData, void*);
    static void on_declaration(CXClientData, const CXIdxDeclInfo*);
    static void on_declaration_reference(CXClientData, const CXIdxEntityRefInfo*);
    static search_result::flags update_document_with_kind(const CXIdxDeclInfo*, document&);
    static void update_document_with_template_kind(CXIdxEntityCXXTemplateKind, document&);
    static void update_document_with_type_size(const CXIdxDeclInfo*, document&);
    static void update_document_with_base_classes(const CXIdxDeclInfo*, document&);

    indexer* const m_indexer;
    std::vector<std::unique_ptr<container_info>> m_containers;
    std::map<declaration_location, docref> m_seen_declarations;
    std::atomic<bool> m_is_cancelled;
};

template <typename... ClientArgs>
inline CXIdxClientContainer worker::update_client_container(ClientArgs&&... args)
{
    auto& ptr = *m_containers.emplace(
        end(m_containers)
      , new container_info{std::forward<ClientArgs>(args)...}
      );
    return CXIdxClientContainer(ptr.get());
}

}}}                                                         // namespace details, index, kate
