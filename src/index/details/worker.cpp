/**
 * \file
 *
 * \brief Class \c kate::index::details::worker (implementation)
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

// Project specific includes
#include "worker.h"
#include "../document_extras.h"
#include "../document.h"
#include "../indexer.h"
#include "../kind.h"
#include "../../clang/kind_of.h"
#include "../../clang/to_string.h"
#include "../../string_cast.h"

// Standard includes
#include <boost/algorithm/string.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/string.hpp>
#include <KDE/KUrl>
#include <KDE/KDebug>
#include <KDE/KLocalizedString>
#include <KDE/KMimeType>
#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QDirIterator>
#include <xapian.h>
#include <set>

namespace kate { namespace index { namespace details { namespace {
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

worker::worker(indexer* const parent)
  : m_indexer{parent}
  , m_is_cancelled{false}
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
    Q_EMIT(finished());
    kDebug(DEBUG_AREA) << "Indexer thread has finished";
}

template <typename... ClientArgs>
inline CXIdxClientContainer worker::update_client_container(ClientArgs&&... args)
{
    auto& ptr = *m_containers.emplace(
        end(m_containers)
      , new container_info{std::forward<ClientArgs>(args)...}
      );
    return CXIdxClientContainer(ptr.get());
}

void worker::handle_file(const QString& filename)
{
    kDebug(DEBUG_AREA) << "Indexing:" << filename;
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
    {
        Q_EMIT(
            message({
                clang::location{}
              , i18nc(
                    "@info/plain"
                  , "Indexing of <filename>%1</filename> finished with errors"
                  , filename
                  )
              , clang::diagnostic_message::type::error
              })
          );
    }
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
        kDebug() << "File extension not recognized: trying to check MIME-type:" << mime->name();
        result = (
            mime->name() != KMimeType::defaultMimeType()
          && CPP_SOURCE_MIME_TYPES.find(mime->name()) != end(CPP_SOURCE_MIME_TYPES)
          ) || false
          ;
    }
    return result;
}

int worker::on_abort_cb(CXClientData, void*)
{
    return 0;
}

void worker::on_diagnostic_cb(CXClientData client_data, CXDiagnosticSet diagnostics, void*)
{
    auto* const wrk = static_cast<worker*>(client_data);
    for (auto i = 0u, num = clang_getNumDiagnosticsInSet(diagnostics); i < num; ++i)
    {
        clang::DCXDiagnostic diag = {clang_getDiagnosticInSet(diagnostics, i)};
        const auto severity = clang_getDiagnosticSeverity(diag);
        if (severity == CXDiagnostic_Ignored)
            continue;

        // Get record type
        clang::diagnostic_message::type type;
        switch (severity)
        {
            case CXDiagnostic_Note:
                type = clang::diagnostic_message::type::info;
                break;
            case CXDiagnostic_Warning:
                type = clang::diagnostic_message::type::warning;
                break;
            case CXDiagnostic_Error:
            case CXDiagnostic_Fatal:
                type = clang::diagnostic_message::type::error;
                break;
            default:
                assert(!"Unexpected severity level! Code review required!");
                // NOTE Fake assign to suppress warning. Normally this
                // code is unreachable
                type = clang::diagnostic_message::type::debug;
                break;
        }

        // Get location
        clang::location loc;
        /// \attention \c Notes have no location attached!?
        if (severity != CXDiagnostic_Note)
        {
            try
            {
                loc = {clang_getDiagnosticLocation(diag)};
            }
            catch (std::exception& e)
            {
                kDebug(DEBUG_AREA) << "indexer diag.fmt: Can't get diagnostic location";
            }
        }
        auto msg = clang::toString(clang_getDiagnosticSpelling(diag));
        Q_EMIT(
            wrk->message(
                {std::move(loc), std::move(msg), type}
              )
          );
    }
}

CXIdxClientFile worker::on_entering_main_file(CXClientData, CXFile file, void*)
{
    auto filename = clang::toString(file);
    kDebug() << "Entering main file: filename=" << filename;
    return static_cast<CXIdxClientFile>(file);
}

CXIdxClientFile worker::on_include_file(CXClientData client_data, const CXIdxIncludedFileInfo* info)
{
#if 0
    char open = info->isAngled ? '<' : '"';
    char close = info->isAngled ? '>' : '"';
    kDebug(DEBUG_AREA) << "#include "
        << open << info->filename << close
        << " at line " << clang::location(info->hashLoc).line();
#endif
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
    return CXIdxClientContainer(wrk->update_client_container(docref{}, std::string{}, std::string{}));
}

void worker::on_declaration(CXClientData client_data, const CXIdxDeclInfo* const info)
{
    clang::location loc;
    try
    {
        loc = clang::location{info->loc};
    }
    catch (...)
    {
        auto name = string_cast<std::string>(info->entityInfo->name);
        CXIdxClientFile file;
        unsigned line;
        unsigned column;
        unsigned offset;
        clang_indexLoc_getFileLocation(info->loc, &file, nullptr, &line, &column, &offset);
        kDebug() << "DECLARATION W/O LOCATION: name=" << name.c_str() << ", line=" << line << ", col=" << column;
        return;
    }
    // Make sure we've not seen it yet
    auto* const wrk = static_cast<worker*>(client_data);
    auto file_id = wrk->m_indexer->m_db.headers_map()[loc.file().toLocalFile()];
    auto decl_loc = declaration_location{file_id, loc.line(), loc.column()};
    /// \todo Track all locations for namespaces and then update
    /// the only document w/ them...
    if (wrk->m_seen_declarations.find(decl_loc) != end(wrk->m_seen_declarations))
        return;

    /// \note Unnamed parameters and anonymous namespaces/classes/structs/enums/unions
    /// have an empty name!
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
#if 0
#ifndef NDEBUG
            if (!boost::equal(before, name))
                kDebug() << "name cleaner: " << before.c_str() << " --> " << name.c_str();
#endif
#endif
        }
        // NOTE Add terms w/ possible stripped name, but have a full name
        // at value slots
#if 0
        doc.add_posting(name, make_term_position(loc));
#else
        doc.add_term(boost::to_lower_copy(name));
#endif
        doc.add_boolean_term(term::XDECL, name);            // Mark the document w/ XDECL prefixed term
        // NOTE Add an original name to the value slot!
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
    auto parent_qname = std::string{};
    if (info->semanticContainer)
    {
        const auto* const container = reinterpret_cast<const container_info* const>(
            clang_index_getClientContainer(info->semanticContainer)
          );
        if (container)
        {
            doc.add_value(value_slot::SEMANTIC_CONTAINER, docref::to_string(container->m_ref));
            parent_qname = container->m_qname;
            if (!parent_qname.empty())
            {
                doc.add_boolean_term(term::XSCOPE, container->m_name);
                doc.add_boolean_term(term::XSCOPE, parent_qname);
                doc.add_value(value_slot::SCOPE, parent_qname);
            }
        }
#if 0
        else
        {
            kDebug(DEBUG_AREA) << "No semantic container for" << name.c_str();
        }
#endif
    }
    if (info->lexicalContainer)
    {
        const auto* const container = reinterpret_cast<const container_info* const>(
            clang_index_getClientContainer(info->lexicalContainer)
          );
        if (container)
            doc.add_value(value_slot::LEXICAL_CONTAINER, docref::to_string(container->m_ref));
#if 0
        else
            kDebug(DEBUG_AREA) << "No lexical container for" << name.c_str();
#endif
    }

    // Get more terms/slots to attach
    auto type_flags = update_decl_document_with_kind(info, doc);
    type_flags.m_decl = true;

    // Attach symbol type. Get aliased type for typedefs, get underlaid type for enums.
    {
        const auto kind = clang::kind_of(*info->entityInfo);
        CXType ct;
        if (kind == CXIdxEntity_CXXTypeAlias || kind == CXIdxEntity_Typedef)
            ct = clang_getTypedefDeclUnderlyingType(info->cursor);
        else if (kind == CXIdxEntity_Enum)
            ct = clang_getEnumDeclIntegerType(info->cursor);
        else
            ct = clang_getCursorType(info->cursor);
        const auto k = clang::kind_of(ct);
        if (k != CXType_Invalid && k != CXType_Unexposed)
        {
            auto type_str = to_string(clang::DCXString{clang_getTypeSpelling(ct)});
            if (!type_str.empty())
                doc.add_value(value_slot::TYPE, type_str);
            // Check some type properties
            if (clang_isPODType(ct))
            {
                doc.add_boolean_term(term::XPOD + "y");
                type_flags.m_pod = true;
            }
            if (clang_isConstQualifiedType(ct))
                type_flags.m_const = true;
            if (clang_isVolatileQualifiedType(ct))
                type_flags.m_volatile = true;
            const auto arity = clang_getNumArgTypes(ct);
            if (arity != -1)
                doc.add_value(value_slot::ARITY, Xapian::sortable_serialise(arity));
        }

        // Get base classes
        const auto can_have_inheritance = kind == CXIdxEntity_Struct
          || kind == CXIdxEntity_CXXClass
          ;
        if (can_have_inheritance)
            update_document_with_base_classes(info, doc);
    }

    // Try to get access specifier for a declaration
    {
        const auto access = clang_getCXXAccessSpecifier(info->cursor);
        switch (access)
        {
            case CX_CXXPublic:
                doc.add_boolean_term(term::XACCESS + "public");
                doc.add_value(value_slot::ACCESS, serialize(unsigned(access)));
                break;
            case CX_CXXProtected:
                doc.add_boolean_term(term::XACCESS + "protected");
                doc.add_value(value_slot::ACCESS, serialize(unsigned(access)));
                break;
            case CX_CXXPrivate:
                doc.add_boolean_term(term::XACCESS + "private");
                doc.add_value(value_slot::ACCESS, serialize(unsigned(access)));
                break;
            case CX_CXXInvalidAccessSpecifier:
            default:
                break;
        }
    }

    if ((type_flags.m_redecl = bool(info->isRedeclaration)))
        doc.add_boolean_term(term::XREDECLARATION + "y");

    if ((type_flags.m_implicit = bool(info->isImplicit)))
        doc.add_boolean_term(term::XIMPLICIT + "y");

    // Attach collected type flags finally
    if (type_flags.m_flags_as_int)
        doc.add_value(value_slot::FLAGS, serialize(type_flags.m_flags_as_int));

    // Add the document to the DB finally
    auto document_id = wrk->m_indexer->m_db.add_document(doc);
    auto ref = docref{database_id, document_id};
    wrk->m_seen_declarations[decl_loc] = ref;               // Mark it as seen declaration

    // Make a new container if necessary
    if (info->declAsContainer)
    {
        if (parent_qname.empty())
            parent_qname = name;
        else
            parent_qname += "::" + name;
        clang_index_setClientContainer(
            info->declAsContainer
          , wrk->update_client_container(ref, name, parent_qname)
          );
    }
}

/// \todo Deduplicate code w/ \c on_declaration
void worker::on_declaration_reference(CXClientData client_data, const CXIdxEntityRefInfo* const info)
{
    clang::location loc;
    try
    {
        loc = clang::location{info->loc};
    }
    catch (...)
    {
        auto name = string_cast<std::string>(info->referencedEntity->name);
        CXIdxClientFile file;
        unsigned line;
        unsigned column;
        unsigned offset;
        clang_indexLoc_getFileLocation(info->loc, &file, nullptr, &line, &column, &offset);
        kDebug() << "REFERENCE W/O LOCATION: name=" << name.c_str() << ", line=" << line << ", col=" << column;
        return;
    }
    // Make sure we've not seen it yet
    auto* const wrk = static_cast<worker*>(client_data);
    auto file_id = wrk->m_indexer->m_db.headers_map()[loc.file().toLocalFile()];
    auto decl_loc = declaration_location{file_id, loc.line(), loc.column()};
    /// \todo Track all locations for namespaces and then update
    /// the only document w/ them...
    if (wrk->m_seen_declarations.find(decl_loc) != end(wrk->m_seen_declarations))
        return;

    /// \todo Are empty names possible?
    /// \todo Remove possible spaces from \c name?
    auto name = string_cast<std::string>(info->referencedEntity->name);
    assert("FIXME: Empty reference name detected" && !name.empty());

    auto ct = clang_getCursorType(info->cursor);
    const auto t = toString(clang::DCXString{clang_getTypeSpelling(ct)});

#if 0
    const auto k = clang::kind_of(ct);
    kDebug() << "found reference: name=" << name.c_str() << ", kind=" << clang::toString(k) << ", type=" << t;
#endif

    // Create a new document for declaration and attach all required value slots and terms
    auto doc = document{};

    // Add terms related to name
#if 0
    doc.add_posting(name, make_term_position(loc));
#else
    doc.add_term(boost::to_lower_copy(name));
#endif
    doc.add_boolean_term(term::XREF, name);                 // Mark the document w/ XREF prefixed term
    doc.add_value(value_slot::NAME, name);

    // Add location terms
    doc.add_value(value_slot::LINE, Xapian::sortable_serialise(loc.line()));
    doc.add_value(value_slot::COLUMN, Xapian::sortable_serialise(loc.column()));
    doc.add_value(value_slot::FILE, Xapian::sortable_serialise(file_id));
    const auto database_id = wrk->m_indexer->m_db.id();
    doc.add_value(value_slot::DBID, serialize(database_id));

    if (info->container)
    {
        const auto* const container = reinterpret_cast<const container_info* const>(
            clang_index_getClientContainer(info->container)
          );
        if (container)
        {
            doc.add_value(value_slot::LEXICAL_CONTAINER, docref::to_string(container->m_ref));
            const auto& parent_qname = container->m_qname;
#if 0
            kDebug(DEBUG_AREA) << "Found parent container:" << parent_qname.c_str();
#endif
            if (!parent_qname.empty())
            {
                doc.add_boolean_term(term::XSCOPE, container->m_name);
                doc.add_boolean_term(term::XSCOPE, parent_qname);
                doc.add_value(value_slot::SCOPE, parent_qname);
            }
        }
#if 0
        else
        {
            kDebug(DEBUG_AREA) << "No parent container for" << name.c_str();
        }
#endif
    }

    auto type_flags = update_ref_document_with_kind(info, doc);
    // Attach collected type flags finally
    if (type_flags.m_flags_as_int)
        doc.add_value(value_slot::FLAGS, serialize(type_flags.m_flags_as_int));

    // Add the document to the DB finally
    auto document_id = wrk->m_indexer->m_db.add_document(doc);
    auto ref = docref{database_id, document_id};
    wrk->m_seen_declarations[decl_loc] = ref;               // Mark it as seen reference
}

search_result::flags worker::update_decl_document_with_kind(
    const CXIdxDeclInfo* const info
  , document& doc
  )
{
    auto type_flags = search_result::flags{};

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
            update_document_with_type_size(info, doc);
            break;
        case CXIdxEntity_CXXTypeAlias:
            doc.add_boolean_term(term::XKIND + "type-alias");
            doc.add_value(value_slot::KIND, serialize(kind::TYPE_ALIAS));
            update_document_with_template_kind(info->entityInfo->templateKind, doc);
            update_document_with_type_size(info, doc);
            break;
        case CXIdxEntity_Struct:
            doc.add_boolean_term(term::XKIND + "struct");
            doc.add_value(value_slot::KIND, serialize(kind::STRUCT));
            update_document_with_template_kind(info->entityInfo->templateKind, doc);
            update_document_with_type_size(info, doc);
            break;
        case CXIdxEntity_CXXClass:
            doc.add_boolean_term(term::XKIND + "class");
            doc.add_value(value_slot::KIND, serialize(kind::CLASS));
            update_document_with_template_kind(info->entityInfo->templateKind, doc);
            update_document_with_type_size(info, doc);
            break;
        case CXIdxEntity_Union:
            doc.add_boolean_term(term::XKIND + "union");
            doc.add_value(value_slot::KIND, serialize(kind::UNION));
            update_document_with_type_size(info, doc);
            break;
        case CXIdxEntity_Enum:
            doc.add_boolean_term(term::XKIND + "enum");
            doc.add_value(value_slot::KIND, serialize(kind::ENUM));
            update_document_with_type_size(info, doc);
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
            if (clang_CXXMethod_isVirtual(info->cursor))
            {
                doc.add_boolean_term(term::XVIRTUAL + "y");
                type_flags.m_virtual = true;
            }
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
            if (clang_CXXMethod_isVirtual(info->cursor))
            {
                doc.add_boolean_term(term::XVIRTUAL + "y");
                type_flags.m_virtual = true;
            }
            break;
        case CXIdxEntity_CXXConversionFunction:
            doc.add_boolean_term(term::XKIND + "fn");
            doc.add_boolean_term(term::XKIND + "conversion");
            doc.add_value(value_slot::KIND, serialize(kind::CONVERSTION));
            update_document_with_template_kind(info->entityInfo->templateKind, doc);
            if (clang_CXXMethod_isVirtual(info->cursor))
            {
                doc.add_boolean_term(term::XVIRTUAL + "y");
                type_flags.m_virtual = true;
            }
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
                /// \bug clang 3.3 (at lest! other version I guess also affected)
                /// some times (noticed for <tt>auto& variable</tt>) may crash
                /// on getting \c sizeof type of the cursor... so do not evn try
                /// do get it...
                /// \todo Bug investigation required.
#if 0
                update_document_with_type_size(info, doc);
#endif
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
                type_flags.m_bit_field = true;
            }
            else
            {
                doc.add_value(value_slot::KIND, serialize(kind::FIELD));
                update_document_with_type_size(info, doc);
            }
            break;
        default:
            break;
    }
    return type_flags;
}

search_result::flags worker::update_ref_document_with_kind(
    const CXIdxEntityRefInfo* const info
  , document& doc
  )
{
    auto type_flags = search_result::flags{};

    switch (clang::kind_of(*info->referencedEntity))
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
            update_document_with_template_kind(info->referencedEntity->templateKind, doc);
            break;
        case CXIdxEntity_Struct:
            doc.add_boolean_term(term::XKIND + "struct");
            doc.add_value(value_slot::KIND, serialize(kind::STRUCT));
            update_document_with_template_kind(info->referencedEntity->templateKind, doc);
            break;
        case CXIdxEntity_CXXClass:
            doc.add_boolean_term(term::XKIND + "class");
            doc.add_value(value_slot::KIND, serialize(kind::CLASS));
            update_document_with_template_kind(info->referencedEntity->templateKind, doc);
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
            update_document_with_template_kind(info->referencedEntity->templateKind, doc);
            break;
        case CXIdxEntity_CXXStaticMethod:
            doc.add_boolean_term(term::XSTATIC + "y");
            type_flags.m_static = true;
            // ATTENTION Fall into the next (CXIdxEntity_CXXInstanceMethod) case...
        case CXIdxEntity_CXXInstanceMethod:
            doc.add_boolean_term(term::XKIND + "fn");
            doc.add_boolean_term(term::XKIND + "method");
            doc.add_value(value_slot::KIND, serialize(kind::METHOD));
            update_document_with_template_kind(info->referencedEntity->templateKind, doc);
            if (clang_CXXMethod_isVirtual(info->cursor))
            {
                doc.add_boolean_term(term::XVIRTUAL + "y");
                type_flags.m_virtual = true;
            }
            break;
        case CXIdxEntity_CXXConstructor:
            doc.add_boolean_term(term::XKIND + "fn");
            doc.add_boolean_term(term::XKIND + "ctor");
            doc.add_value(value_slot::KIND, serialize(kind::CONSTRUCTOR));
            update_document_with_template_kind(info->referencedEntity->templateKind, doc);
            break;
        case CXIdxEntity_CXXDestructor:
            doc.add_boolean_term(term::XKIND + "fn");
            doc.add_boolean_term(term::XKIND + "dtor");
            doc.add_value(value_slot::KIND, serialize(kind::DESTRUCTOR));
            if (clang_CXXMethod_isVirtual(info->cursor))
            {
                doc.add_boolean_term(term::XVIRTUAL + "y");
                type_flags.m_virtual = true;
            }
            break;
        case CXIdxEntity_CXXConversionFunction:
            doc.add_boolean_term(term::XKIND + "fn");
            doc.add_boolean_term(term::XKIND + "conversion");
            doc.add_value(value_slot::KIND, serialize(kind::CONVERSTION));
            update_document_with_template_kind(info->referencedEntity->templateKind, doc);
            if (clang_CXXMethod_isVirtual(info->cursor))
            {
                doc.add_boolean_term(term::XVIRTUAL + "y");
                type_flags.m_virtual = true;
            }
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
                type_flags.m_bit_field = true;
            }
            else doc.add_value(value_slot::KIND, serialize(kind::FIELD));
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

void worker::update_document_with_type_size(
    const CXIdxDeclInfo* info
  , document& doc
  )
{
    auto ct = clang_getCursorType(info->cursor);
    const auto k = clang::kind_of(ct);
    if (k != CXType_Invalid && k != CXType_Unexposed)
    {
        // Get sizeof
        const auto size = clang_Type_getSizeOf(ct);
        if (0 < size)
            doc.add_value(value_slot::SIZEOF, Xapian::sortable_serialise(size));
        // Get alignof
        const auto align = clang_Type_getAlignOf(ct);
        if (0 < align)
            doc.add_value(value_slot::ALIGNOF, Xapian::sortable_serialise(align));
        /// \todo Get offset of, if applicable
    }
}

void worker::update_document_with_base_classes(const CXIdxDeclInfo* info, document& doc)
{
    const auto* class_info = clang_index_getCXXClassDeclInfo(info);
    std::list<std::string> bases;
    if (class_info)
    {
        for (auto i = 0u; i < class_info->numBases; ++i)
        {
            if (class_info->bases[i]->base && class_info->bases[i]->base->name)
            {
                auto name = string_cast<std::string>(class_info->bases[i]->base->name);
                assert("Unnamed base class?" && !name.empty());
                doc.add_boolean_term(term::XBASE_CLASS, name);

                auto inheritance_term = std::string{};
                const auto is_virtual = clang_isVirtualBase(class_info->bases[i]->cursor);
                if (is_virtual)
                    inheritance_term = "virtual";
                const auto access = clang_getCXXAccessSpecifier(class_info->bases[i]->cursor);
                auto access_term = std::string{};
                switch (access)
                {
                    case CX_CXXPublic:
                        access_term = "public";
                        break;
                    case CX_CXXProtected:
                        access_term = "protected";
                        break;
                    case CX_CXXPrivate:
                        access_term = "private";
                        break;
                    default:
                        break;
                }
                if (!access_term.empty())
                {
                    if (inheritance_term.empty())
                        name = access_term + " " + name;
                    else
                    {
                        doc.add_boolean_term(term::XINHERITANCE + inheritance_term + "-" + access_term);
                        name = inheritance_term + " " + access_term + " " + name;
                    }
                    // NOTE Also add inheritance term w/o 'virutal' specifier,
                    // so "inh:public" for example will give results also for
                    // 'virtual' inheritance...
                    doc.add_boolean_term(term::XINHERITANCE + access_term);
                }
                bases.emplace_back(std::move(name));
            }
#ifndef NDEBUG
            else
            {
                kDebug(DEBUG_AREA) << "nullptr in a base class list or class name!?";
            }
#endif
        }
    }
    if (!bases.empty())
    {
        // serialize bases list into a value slot
        std::stringstream ss{std::ios_base::out | std::ios_base::binary};
        boost::archive::binary_oarchive oa{ss};
        oa << bases;
        doc.add_value(value_slot::BASES, ss.str());
    }
}

}}}                                                         // namespace details, index, kate
