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
#include <src/clang/disposable.h>
#include <src/clang/kind_of.h>
#include <src/clang/location.h>
#include <src/clang/to_string.h>

// Standard includes
#include <KDE/KDebug>
#include <KDE/KMimeType>
#include <QtCore/QDirIterator>
#include <xapian/document.h>
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
 * - \c SEMANTIC_PARENT -- ID of the semantic parent document
 * - \c LEXICAL_PARENT -- ID of the lexical parent document
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

struct indexer_data
{
    std::string m_main_file;
};

inline Xapian::termpos make_term_position(const clang::location& loc)
{
    return loc.line() * 1000 + loc.column();
}

}                                                           // anonymous namespace

//BEGIN worker members
int worker::on_abort_cb(CXClientData client_data, void*)
{
    auto* const wrkr = static_cast<worker*>(client_data);
    return 0;
}

void worker::on_diagnostic_cb(CXClientData, CXDiagnosticSet, void*)
{
}

CXIdxClientFile worker::on_entering_main_file(CXClientData client_data, CXFile file, void*)
{
    auto* const wrkr = static_cast<worker*>(client_data);
    kDebug(DEBUG_AREA) << "Main file:" << clang::toString(file);
    return static_cast<CXIdxClientFile>(file);
}

CXIdxClientFile worker::on_include_file(CXClientData client_data, const CXIdxIncludedFileInfo* info)
{
    auto* const wrkr = static_cast<worker*>(client_data);
    return static_cast<CXIdxClientFile>(info->file);
}

CXIdxClientASTFile worker::on_include_ast_file(CXClientData client_data, const CXIdxImportedASTFileInfo* info)
{
    auto* const wrkr = static_cast<worker*>(client_data);
    return static_cast<CXIdxClientFile>(info->file);
}

CXIdxClientContainer worker::on_translation_unit(CXClientData client_data, void*)
{
    auto* const wrkr = static_cast<worker*>(client_data);
    return CXIdxClientContainer("TU");
}

void worker::on_declaration(CXClientData client_data, const CXIdxDeclInfo* info)
{
    auto* const wrkr = static_cast<worker*>(client_data);
    auto loc = clang::location{info->loc};
    auto doc = Xapian::Document{};
    doc.add_posting(info->entityInfo->name, make_term_position(loc));
    doc.add_boolean_term(term::XDECL);
    wrkr->m_indexer->m_db.add_document(doc);
}

void worker::on_declaration_reference(CXClientData client_data, const CXIdxEntityRefInfo* info)
{
}

worker::worker(indexer* const parent)
  : m_indexer(parent)
{
}

void worker::request_cancel()
{
    m_is_cancelled = true;
}

bool worker::is_cancelled() const
{
    return m_is_cancelled;
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

void worker::handle_file(const QString& filename)
{
    kDebug(DEBUG_AREA) << "Indexing" << filename;
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
        Q_EMIT(error(clang::location{}, "Indexer finished with errors"));
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
#if 0
    kDebug(DEBUG_AREA) << "Dispatch" << filename;
#endif
    if (fi.isDir())
        handle_directory(filename);
    else if (fi.isFile() && is_look_like_cpp_source(fi))
        handle_file(filename);
    else
        kDebug(DEBUG_AREA) << filename << "is not a suitable file or directory!";
    return false;
}

void worker::process()
{
    kDebug(DEBUG_AREA) << "Indexer thread has started";
    for (const auto& target : m_indexer->m_targets)
        if (dispatch_target(target))
            break;
    kDebug(DEBUG_AREA) << "Indexer thread has finished";
    Q_EMIT(finished());
}
//END worker members

//BEGIN indexer members
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
    Q_EMIT(finished());
}

void indexer::indexing_uri_slot(QString file)
{
    Q_EMIT(indexing_uri(file));
}
//END indexer members
}}                                                          // namespace index, kate
