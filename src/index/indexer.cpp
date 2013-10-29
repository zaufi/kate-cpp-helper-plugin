/**
 * \file
 *
 * \brief Class \c kate::index::indexer (implementation)
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

// Project specific includes
#include <src/index/indexer.h>
#include <src/clang/kind_of.h>
#include <src/clang/to_string.h>

// Standard includes
#include <KDE/KDebug>
#include <KDE/KMimeType>
#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QDirIterator>
#include <QtCore/QThread>
#include <xapian/document.h>
#include <xapian/queryparser.h>
#include <set>

namespace kate { namespace index {
/**
 * \class kate::index::indexer
 *
 * Scope containers:
 * - file
 * - namespace
 * - class/struct/union/enum
 * - function
 *
 * Every scope is a document.
 * Document contains a set of postings, boolean terms and values.
 * For every declaration or reference a new document will be created.
 * Possible boolean terms are:
 * - \c XDECL -- to indicate a declaration
 * - \c XREF -- to indicate a reference
 *
 * Possible values:
 * - \c REF_TO -- document ID of a declaration for a reference
 * - \c KIND -- cursor kind (function, typedef, class/struct, etc...)
 * - \c SEMANTIC_CONTAINER -- ID of the semantic parent document
 * - \c LEXICAL_CONTAINER -- ID of the lexical parent document
 * - \c LINE -- line number
 * - \c COLUMN -- column
 * - \c FILE -- filename
 */

namespace {
std::set<QString> TYPICAL_CPP_EXTENSIONS = {
    "h", "hh", "hpp", "hxx", "inl", "tcc"
  , "c", "cc", "cpp", "cxx"
};
std::set<QString> CPP_SOURCE_MIME_TYPES = {
    "text/x-csrc"
  , "text/x-c++src"
  , "text/x-chdr"
  , "text/x-c++hdr"
};

inline Xapian::termpos make_term_position(const clang::location& loc)
{
    return loc.line() * 1000 + loc.column();
}

}                                                           // anonymous namespace

//BEGIN worker members
struct worker::container_info
{
    docref m_ref;                                           ///< ref to a container ID in a current DB
};

worker::worker(indexer* const parent)
  : m_indexer{parent}
  , m_is_cancelled{false}
{
}

worker::~worker()
{
}

void worker::request_cancel()
{
    kDebug() << "Indexing worker: Got stop request!";
    m_is_cancelled = true;
}

bool worker::is_cancelled() const
{
    return m_is_cancelled;
}

void worker::process()
{
    kDebug(DEBUG_AREA) << "Indexer thread has started";
    for (const auto& target : m_indexer->m_targets)
        if (dispatch_target(target))
            break;
    kDebug(DEBUG_AREA) << "Indexer thread has finished";
    m_indexer->m_db.commit();
    Q_EMIT(finished());
}

void worker::handle_file(const QString& filename)
{
    kDebug(DEBUG_AREA) << "Indexing" << filename;
    Q_EMIT(indexing_uri(filename));

    clang::DCXIndexAction action = {clang_IndexAction_create(m_indexer->m_index)};
    IndexerCallbacks index_callbacks = {
        &worker::on_abort_cb
      , &worker::on_diagnostic_cb
      , &worker::on_entering_main_file
      , &worker::on_include_file
      , &worker::on_include_ast_file
      , &worker::on_translation_unit
      , &worker::on_declaration
      , &worker::on_declaration_reference
    };
    auto result = clang_indexSourceFile(
        action
      , this
      , &index_callbacks
      , sizeof(index_callbacks)
        // CXIndexOpt_SuppressRedundantRefs
        // CXIndexOpt_SkipParsedBodiesInSession
      , CXIndexOpt_IndexFunctionLocalSymbols
      , filename.toUtf8().constData()
      , m_indexer->m_options.data()
      , m_indexer->m_options.size()
      , nullptr
      , 0
      , nullptr
      , clang_defaultEditingTranslationUnitOptions()        /// \todo Use TranslationUnit class
      );
    if (result)
        Q_EMIT(error(clang::location{}, QString("Errors on indexing %1").arg(filename)));
}

void worker::handle_directory(const QString& directory)
{
    for (
        QDirIterator dir_it = {
            directory
          , QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::CaseSensitive
          , QDirIterator::FollowSymlinks
          }
      ; dir_it.hasNext()
      ;
      )
    {
        dir_it.next();
        if (dispatch_target(dir_it.fileInfo()))
            break;
        //
        QCoreApplication::processEvents();
    }
}

inline bool worker::dispatch_target(const KUrl& url)
{
    return dispatch_target(QFileInfo{url.toLocalFile()});
}

bool worker::dispatch_target(const QFileInfo& fi)
{
    if (is_cancelled())
    {
        kDebug(DEBUG_AREA) << "Cancel requested: exiting indexer thread...";
        return true;
    }

    const auto& filename = fi.canonicalFilePath();
    if (fi.isDir())
        handle_directory(filename);
    else if (fi.isFile() && is_look_like_cpp_source(fi))
        handle_file(filename);
    else
        kDebug(DEBUG_AREA) << filename << "is not a suitable file or directory!";
    return false;
}

bool worker::is_look_like_cpp_source(const QFileInfo& fi)
{
    auto result = true;
    if (TYPICAL_CPP_EXTENSIONS.find(fi.suffix()) == end(TYPICAL_CPP_EXTENSIONS))
    {
        // Try to use Mime database to detect by file content
        auto mime = KMimeType::findByFileContent(fi.canonicalFilePath());
        result = (
            mime->name() != KMimeType::defaultMimeType()
          && CPP_SOURCE_MIME_TYPES.find(mime->name()) != end(CPP_SOURCE_MIME_TYPES)
          ) || false
          ;
    }
    return result;
}

CXIdxClientContainer worker::update_client_container(const docref id)
{
    auto& ptr = *m_containers.emplace(end(m_containers), new container_info{id});
    return CXIdxClientContainer(ptr.get());
}

int worker::on_abort_cb(CXClientData client_data, void*)
{
    auto* const wrk = static_cast<worker*>(client_data);
    return 0;
}

void worker::on_diagnostic_cb(CXClientData, CXDiagnosticSet, void*)
{
}

CXIdxClientFile worker::on_entering_main_file(CXClientData client_data, CXFile file, void*)
{
    auto* const wrk = static_cast<worker*>(client_data);
    kDebug(DEBUG_AREA) << "Main file:" << clang::toString(file);
    return static_cast<CXIdxClientFile>(file);
}

CXIdxClientFile worker::on_include_file(CXClientData client_data, const CXIdxIncludedFileInfo* info)
{
    auto* const wrk = static_cast<worker*>(client_data);
    return static_cast<CXIdxClientFile>(info->file);
}

CXIdxClientASTFile worker::on_include_ast_file(CXClientData client_data, const CXIdxImportedASTFileInfo* info)
{
    auto* const wrk = static_cast<worker*>(client_data);
    return static_cast<CXIdxClientFile>(info->file);
}

CXIdxClientContainer worker::on_translation_unit(CXClientData client_data, void*)
{
    auto* const wrk = static_cast<worker*>(client_data);
    return CXIdxClientContainer(wrk->update_client_container(docref{}));
}

void worker::on_declaration(CXClientData client_data, const CXIdxDeclInfo* info)
{
    auto loc = clang::location{info->loc};
    if (!info->entityInfo->name)
    {
        kDebug(DEBUG_AREA) << loc << ": it seems here is anonymous declaration";
        return;
    }
    // Make sure we've not seen it yet
    auto* const wrk = static_cast<worker*>(client_data);
    auto file_id = wrk->m_indexer->m_db.headers_map()[loc.file().toLocalFile()];
    auto decl_loc = declaration_location{file_id, loc.line(), loc.column()};
    if (wrk->m_seen_declarations.find(decl_loc) != end(wrk->m_seen_declarations))
    {
        //kDebug(DEBUG_AREA) << loc << ": " << info->entityInfo->name << "seen it!";
        return;
    }

    /// \note Unnamed parameters have empty name (only type, available via ref)
    auto name = QString{info->entityInfo->name};
    if (name.isEmpty())
    {
        kDebug(DEBUG_AREA) << loc << ": it seems here is unnamed parameter declaration";
        return;
    }

    kDebug() << loc << "declaration of" << QString(info->entityInfo->name);

    // Create a new document for declaration and attach all required value slots and terms
    auto doc = Xapian::Document{};
    doc.add_posting(info->entityInfo->name, make_term_position(loc));
    doc.add_boolean_term(term::XDECL);                      // Mark document w/ XDECL term
    doc.add_value(value_slot::NAME, info->entityInfo->name);
    doc.add_value(value_slot::LINE, Xapian::sortable_serialise(loc.line()));
    doc.add_value(value_slot::COLUMN, Xapian::sortable_serialise(loc.column()));
    doc.add_value(value_slot::FILE, Xapian::sortable_serialise(file_id));
    const auto database_id = wrk->m_indexer->m_db.id();
    doc.add_value(value_slot::DBID, Xapian::sortable_serialise(database_id));
    if (info->semanticContainer)
    {
        const auto* const container = reinterpret_cast<const container_info* const>(info->semanticContainer);
        doc.add_value(value_slot::SEMANTIC_CONTAINER, docref::to_string(container->m_ref));
    }
    if (info->lexicalContainer)
    {
        const auto* const container = reinterpret_cast<const container_info* const>(info->lexicalContainer);
        doc.add_value(value_slot::LEXICAL_CONTAINER, docref::to_string(container->m_ref));
    }
    if (info->declAsContainer)
        doc.add_boolean_term(term::XCONTAINER);
#if 0
    /// \note Don't remember when it possible...
    if (info->isRedeclaration)
#endif

    // Add the document to the DB finally
    auto document_id = wrk->m_indexer->m_db.add_document(doc);
    auto ref = docref{database_id, document_id};
    wrk->m_seen_declarations[decl_loc] = ref;               // Mark it as seen declaration

    // Make a new container if necessary
    if (info->declAsContainer)
        clang_index_setClientContainer(
            info->declAsContainer
          , wrk->update_client_container(ref)
          );
}

void worker::on_declaration_reference(CXClientData client_data, const CXIdxEntityRefInfo* info)
{
}
//END worker members

//BEGIN indexer members
indexer::~indexer()
{
    if (m_worker_thread)
    {
        stop();
        m_worker_thread->quit();
        m_worker_thread->wait();
    }
}

void indexer::start()
{
    QThread* t = new QThread{};
    worker* w = new worker{this};
    connect(w, SIGNAL(error(clang::location, QString)), this, SLOT(error_slot(clang::location, QString)));
    connect(w, SIGNAL(indexing_uri(QString)), this, SLOT(indexing_uri_slot(QString)));
    connect(w, SIGNAL(finished()), this, SLOT(finished_slot()));
    connect(this, SIGNAL(stopping()), w, SLOT(request_cancel()));

    connect(w, SIGNAL(finished()), t, SLOT(quit()));
    connect(w, SIGNAL(finished()), w, SLOT(deleteLater()));

    connect(t, SIGNAL(started()), w, SLOT(process()));
    connect(t, SIGNAL(finished()), t, SLOT(deleteLater()));
    w->moveToThread(t);
    t->start();
    m_worker_thread = t;
}

void indexer::stop()
{
    kDebug(DEBUG_AREA) << "Emitting STOP!";
    Q_EMIT(stopping());
}

void indexer::error_slot(clang::location loc, QString str)
{
    Q_EMIT(error(loc, str));
}

void indexer::finished_slot()
{
    m_worker_thread = nullptr;
    Q_EMIT(finished());
}

void indexer::indexing_uri_slot(QString file)
{
    Q_EMIT(indexing_uri(file));
}
//END indexer members
}}                                                          // namespace index, kate
