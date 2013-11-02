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
#include <src/index/kind.h>
#include <src/string_cast.h>

// Standard includes
#include <boost/algorithm/string.hpp>
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
      , m_indexer->m_indexing_options
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

int worker::on_abort_cb(CXClientData, void*)
{
    return 0;
}

void worker::on_diagnostic_cb(CXClientData, CXDiagnosticSet, void*)
{
}

CXIdxClientFile worker::on_entering_main_file(CXClientData client_data, CXFile file, void*)
{
    auto* const wrk = static_cast<worker*>(client_data);
    Q_UNUSED(wrk);
    kDebug(DEBUG_AREA) << "Main file:" << clang::toString(file);
    return static_cast<CXIdxClientFile>(file);
}

CXIdxClientFile worker::on_include_file(CXClientData client_data, const CXIdxIncludedFileInfo* info)
{
    auto* const wrk = static_cast<worker*>(client_data);
    Q_UNUSED(wrk);
    return static_cast<CXIdxClientFile>(info->file);
}

CXIdxClientASTFile worker::on_include_ast_file(CXClientData client_data, const CXIdxImportedASTFileInfo* info)
{
    auto* const wrk = static_cast<worker*>(client_data);
    Q_UNUSED(wrk);
    return static_cast<CXIdxClientFile>(info->file);
}

CXIdxClientContainer worker::on_translation_unit(CXClientData client_data, void*)
{
    auto* const wrk = static_cast<worker*>(client_data);
    return CXIdxClientContainer(wrk->update_client_container(docref{}));
}

void worker::on_declaration(CXClientData client_data, const CXIdxDeclInfo* const info)
{
    auto loc = clang::location{info->loc};
    // Make sure we've not seen it yet
    auto* const wrk = static_cast<worker*>(client_data);
    auto file_id = wrk->m_indexer->m_db.headers_map()[loc.file().toLocalFile()];
    auto decl_loc = declaration_location{file_id, loc.line(), loc.column()};
    /// \todo Track all locations for namespaces and then update
    /// the only document w/ them...
    if (wrk->m_seen_declarations.find(decl_loc) != end(wrk->m_seen_declarations))
        return;

    /// \note Unnamed parameters and anonymous namespaces/structs/unions have an empty name!
    /// \todo Remove possible spaces from \c name?
    auto name = string_cast<std::string>(info->entityInfo->name);

    // Create a new document for declaration and attach all required value slots and terms
    auto doc = document{};
    // Attach terms related to name
    if (!name.empty())
    {
        // Try to remove any template parameters from the name.
        // For example constructors/descturctors have them.
        const auto kind = clang::kind_of(*info->entityInfo);
        const auto try_remove_template_params =
            kind == CXIdxEntity_CXXConstructor
          || kind == CXIdxEntity_CXXDestructor
          || kind == CXIdxEntity_CXXInstanceMethod
          || kind == CXIdxEntity_CXXConversionFunction
          || kind == CXIdxEntity_Function
          ;
        if (try_remove_template_params)
        {
#ifndef NDEBUG
            auto before = name;
#endif
            const auto tpos = name.find('<');
            if (tpos != std::string::npos && boost::ends_with(name, ">"))
                name.erase(tpos);
#ifndef NDEBUG
            if (!boost::equal(before, name))
                kDebug() << "name cleaner: " << before.c_str() << " --> " << name.c_str();
#endif
        }
        // NOTE Add terms w/ possible stripped name, but have a full name
        // at value slots
        doc.add_posting(name, make_term_position(loc));
        doc.add_boolean_term(term::XDECL + name);           // Mark the document w/ XDECL prefixed term
        doc.add_value(value_slot::NAME, info->entityInfo->name);
    }
    else
    {
        doc.add_boolean_term(term::XANONYMOUS + "y");
    }
    doc.add_value(value_slot::LINE, Xapian::sortable_serialise(loc.line()));
    doc.add_value(value_slot::COLUMN, Xapian::sortable_serialise(loc.column()));
    doc.add_value(value_slot::FILE, Xapian::sortable_serialise(file_id));
    const auto database_id = wrk->m_indexer->m_db.id();
    doc.add_value(value_slot::DBID, serialize(database_id));
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

    // Get more terms/slots to attach
    auto type_flags = update_document_with_kind(info, doc);
    if (type_flags.m_flags_as_int)
        doc.add_value(value_slot::FLAGS, serialize(type_flags.m_flags_as_int));

    // Attach symbol type. Get aliasd type for typedefs.
    {
        const auto kind = clang::kind_of(*info->entityInfo);
        const auto is_typedef = kind == CXIdxEntity_CXXTypeAlias || kind == CXIdxEntity_Typedef;
        CXType ct;
        if (is_typedef)
            ct = clang_getTypedefDeclUnderlyingType(info->cursor);
        else if (kind == CXIdxEntity_Enum)
            ct = clang_getEnumDeclIntegerType(info->cursor);
        else
            ct = clang_getCursorType(info->cursor);
        if (clang::kind_of(ct) != CXType_Invalid || clang::kind_of(ct) != CXType_Unexposed)
        {
            auto type_str = to_string(clang::DCXString{clang_getTypeSpelling(ct)});
            if (!type_str.empty())
                doc.add_value(value_slot::TYPE, type_str);
        }
    }

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

void worker::on_declaration_reference(CXClientData client_data, const CXIdxEntityRefInfo* const info)
{
    auto* const wrk = static_cast<worker*>(client_data);
    Q_UNUSED(wrk);
    Q_UNUSED(info);
}

search_result::flags worker::update_document_with_kind(const CXIdxDeclInfo* info, document& doc)
{
    search_result::flags type_flags;

    switch (clang::kind_of(*info->entityInfo))
    {
        case CXIdxEntity_CXXNamespace:
            doc.add_boolean_term(term::XKIND + "ns");
            doc.add_value(value_slot::KIND, serialize(kind::NAMESPACE));
            break;
        case CXIdxEntity_CXXNamespaceAlias:
            doc.add_boolean_term(term::XKIND + "ns-alias");
            doc.add_value(value_slot::KIND, serialize(kind::NAMESPACE_ALIAS));
            break;
        case CXIdxEntity_Typedef:
            doc.add_boolean_term(term::XKIND + "typedef");
            doc.add_value(value_slot::KIND, serialize(kind::TYPEDEF));
            break;
        case CXIdxEntity_CXXTypeAlias:
            doc.add_boolean_term(term::XKIND + "type-alias");
            doc.add_value(value_slot::KIND, serialize(kind::TYPE_ALIAS));
            update_document_with_template_kind(info->entityInfo->templateKind, doc);
            break;
        case CXIdxEntity_Struct:
            doc.add_boolean_term(term::XKIND + "struct");
            doc.add_value(value_slot::KIND, serialize(kind::STRUCT));
            update_document_with_template_kind(info->entityInfo->templateKind, doc);
            break;
        case CXIdxEntity_CXXClass:
            doc.add_boolean_term(term::XKIND + "class");
            doc.add_value(value_slot::KIND, serialize(kind::CLASS));
            update_document_with_template_kind(info->entityInfo->templateKind, doc);
            break;
        case CXIdxEntity_Union:
            doc.add_boolean_term(term::XKIND + "union");
            doc.add_value(value_slot::KIND, serialize(kind::UNION));
            break;
        case CXIdxEntity_Enum:
            doc.add_boolean_term(term::XKIND + "enum");
            doc.add_value(value_slot::KIND, serialize(kind::ENUM));
            break;
        case CXIdxEntity_EnumConstant:
        {
            doc.add_boolean_term(term::XKIND + "enum-const");
            doc.add_value(value_slot::KIND, serialize(kind::ENUM_CONSTANT));
            const auto value = clang_getEnumConstantDeclValue(info->cursor);
            doc.add_value(value_slot::VALUE, Xapian::sortable_serialise(value));
            break;
        }
        case CXIdxEntity_Function:
            doc.add_boolean_term(term::XKIND + "fn");
            doc.add_value(value_slot::KIND, serialize(kind::FUNCTION));
            update_document_with_template_kind(info->entityInfo->templateKind, doc);
            break;
        case CXIdxEntity_CXXStaticMethod:
            doc.add_boolean_term(term::XSTATIC + "y");
            type_flags.m_static = true;
            // ATTENTION Fall into the next (CXIdxEntity_CXXInstanceMethod) case...
        case CXIdxEntity_CXXInstanceMethod:
            doc.add_boolean_term(term::XKIND + "fn");
            doc.add_boolean_term(term::XKIND + "method");
            doc.add_value(value_slot::KIND, serialize(kind::METHOD));
            update_document_with_template_kind(info->entityInfo->templateKind, doc);
            break;
        case CXIdxEntity_CXXConstructor:
            doc.add_boolean_term(term::XKIND + "fn");
            doc.add_boolean_term(term::XKIND + "ctor");
            doc.add_value(value_slot::KIND, serialize(kind::CONSTRUCTOR));
            update_document_with_template_kind(info->entityInfo->templateKind, doc);
            break;
        case CXIdxEntity_CXXDestructor:
            doc.add_boolean_term(term::XKIND + "fn");
            doc.add_boolean_term(term::XKIND + "dtor");
            doc.add_value(value_slot::KIND, serialize(kind::DESTRUCTOR));
            break;
        case CXIdxEntity_CXXConversionFunction:
            doc.add_boolean_term(term::XKIND + "fn");
            doc.add_boolean_term(term::XKIND + "conversion");
            doc.add_value(value_slot::KIND, serialize(kind::CONVERSTION));
            update_document_with_template_kind(info->entityInfo->templateKind, doc);
            break;
        case CXIdxEntity_Variable:
        {
            auto cursor_kind = clang::kind_of(info->cursor);
            if (cursor_kind == CXCursor_ParmDecl)
            {
                doc.add_boolean_term(term::XKIND + "param");
                doc.add_value(value_slot::KIND, serialize(kind::PARAMETER));
            }
            else
            {
                doc.add_boolean_term(term::XKIND + "var");
                doc.add_value(value_slot::KIND, serialize(kind::VARIABLE));
            }
            break;
        }
        case CXIdxEntity_CXXStaticVariable:
            doc.add_boolean_term(term::XSTATIC + "y");
            type_flags.m_static = true;
            // ATTENTION Fall into the next (CXIdxEntity_Field) case...
        case CXIdxEntity_Field:
            doc.add_boolean_term(term::XKIND + "field");
            if (clang_Cursor_isBitField(info->cursor))
            {
                doc.add_boolean_term(term::XKIND + "bitfield");
                doc.add_value(value_slot::KIND, serialize(kind::BITFIELD));
                const auto width = clang_getFieldDeclBitWidth(info->cursor);
                doc.add_value(value_slot::VALUE, Xapian::sortable_serialise(width));
            }
            else
            {
                doc.add_value(value_slot::KIND, serialize(kind::FIELD));
            }
            break;
        default:
            break;
    }
    return type_flags;
}

void worker::update_document_with_template_kind(
    const CXIdxEntityCXXTemplateKind template_kind
  , document& doc
  )
{
    switch (template_kind)
    {
        case CXIdxEntity_Template:
            doc.add_boolean_term(term::XTEMPLATE + "y");
            doc.add_value(
                value_slot::TEMPLATE
              , serialize(unsigned(template_kind))
              );
            break;
        case CXIdxEntity_TemplatePartialSpecialization:
            doc.add_boolean_term(term::XTEMPLATE + "ps");
            doc.add_value(
                value_slot::TEMPLATE
              , serialize(unsigned(template_kind))
              );
            break;
        case CXIdxEntity_TemplateSpecialization:
            doc.add_boolean_term(term::XTEMPLATE + "fs");
            doc.add_value(
                value_slot::TEMPLATE
              , serialize(unsigned(template_kind))
              );
            break;
        default:
            break;
    }
}
//END worker members

//BEGIN indexer members
indexer::indexer(const dbid id, const std::string& path)
  : m_index{clang_createIndex(1, 1)}
  , m_db{id, path}
{
}

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
    m_worker_thread.reset(t);
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
