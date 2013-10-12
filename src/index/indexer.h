/**
 * \file
 *
 * \brief Class \c kate::index::indexer (interface)
 *
 * \date Wed Oct  9 11:17:12 MSK 2013 -- Initial design
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
#include <src/clang/disposable.h>
#include <src/clang/location.h>
#include <src/index/database.h>

// Standard includes
#include <KDE/KUrl>
#include <QtCore/QThread>
#include <QtCore/QFileInfo>
#include <atomic>
#include <vector>

namespace kate { namespace index {

class indexer;

/// Worker class to do an indexer's job
class worker : public QObject
{
    Q_OBJECT

public:
    explicit worker(indexer* const);

    bool is_cancelled() const;

public Q_SLOTS:
    void process();
    void request_cancel();

Q_SIGNALS:
    void indexing_uri(QString);
    void error(clang::location, QString);
    void finished();

private:
    bool dispatch_target(const KUrl&);
    bool dispatch_target(const QFileInfo&);
    void handle_file(const QString&);
    void handle_directory(const QString&);
    bool is_look_like_cpp_source(const QFileInfo&);

    static int on_abort_cb(CXClientData, void*);
    static void on_diagnostic_cb(CXClientData, CXDiagnosticSet, void*);
    static CXIdxClientFile on_entering_main_file(CXClientData, CXFile, void*);
    static CXIdxClientFile on_include_file(CXClientData, const CXIdxIncludedFileInfo*);
    static CXIdxClientASTFile on_include_ast_file(CXClientData, const CXIdxImportedASTFileInfo*);
    static CXIdxClientContainer on_translation_unit(CXClientData, void*);
    static void on_declaration(CXClientData, const CXIdxDeclInfo*);
    static void on_declaration_reference(CXClientData, const CXIdxEntityRefInfo*);

    indexer* const m_indexer;
    std::atomic<bool> m_is_cancelled;
};

/**
 * \brief Class to index C/C++ sources into a searchable databse
 *
 * [More detailed description here]
 *
 */
class indexer : public QObject
{
    Q_OBJECT

public:
    enum class status
    {
        stopped
      , running
    };
    /// Construct an indexer from database path
    explicit indexer(const std::string&);

    indexer& set_compiler_options(std::vector<const char*>&&);
    indexer& add_target(const KUrl&);

public Q_SLOTS:
    void start();
    void stop();

private Q_SLOTS:
    void indexing_uri_slot(QString);
    void finished_slot();
    void error_slot(clang::location, QString);

Q_SIGNALS:
    void indexing_uri(QString);
    void finished();
    void error(clang::location, QString);
    void stopping();

private:
    friend class worker;

    clang::DCXIndex m_index = {clang_createIndex(1, 1)};
    std::vector<const char*> m_options;
    std::vector<KUrl> m_targets;
    database m_db;
};

inline indexer::indexer(const std::string& path)
  : m_db{path}
{
}

inline indexer& indexer::set_compiler_options(std::vector<const char*>&& options)
{
    m_options = std::move(options);
    return *this;
}

inline indexer& indexer::add_target(const KUrl& url)
{
    m_targets.emplace_back(url);
    return *this;
}

}}                                                          // namespace index, kate
