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
#include <src/index/details/worker.h>
#include <src/clang/kind_of.h>
#include <src/clang/to_string.h>
#include <src/index/document.h>
#include <src/index/document_extras.h>
#include <src/index/indexer.h>
#include <src/index/kind.h>
#include <src/string_cast.h>

// Standard includes
#include <boost/algorithm/string.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/string.hpp>
#include <KDE/KUrl>
#include <KDE/KDebug>
#include <KDE/KMimeType>
#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QDirIterator>
#include <xapian/document.h>
#include <xapian/queryparser.h>
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
#if 0
    kDebug(DEBUG_AREA) << "Indexing" << filename;
#endif
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
#if 0
    kDebug(DEBUG_AREA) << "Main file:" << clang::toString(file);
#endif
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
    return CXIdxClientContainer(wrk->update_client_container(docref{}, std::string{}, std::string{}));
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
        doc.add_boolean_term(term::XANONYMOUS);
    }
    doc.add_value(value_slot::LINE, Xapian::sortable_serialise(loc.line()));
    doc.add_value(value_slot::COLUMN, Xapian::sortable_serialise(loc.column()));
    doc.add_value(value_slot::FILE, Xapian::sortable_serialise(file_id));
    const auto database_id = wrk->m_indexer->m_db.id();
    doc.add_value(value_slot::DBID, serialize(database_id));
    auto qname = std::string{};
    if (info->semanticContainer)
    {
        const auto* const container = reinterpret_cast<const container_info* const>(info->semanticContainer);
        if (container->m_name.empty())
            qname = name;
        else
            qname = container->m_qname + "::" + name;
        doc.add_boolean_term(term::XSCOPE + name);
        doc.add_boolean_term(term::XSCOPE + qname);
        kDebug(DEBUG_AREA) << "qname=" << qname.c_str();
        doc.add_value(value_slot::SEMANTIC_CONTAINER, docref::to_string(container->m_ref));
    }
    if (info->lexicalContainer)
    {
        const auto* const container = reinterpret_cast<const container_info* const>(info->lexicalContainer);
        doc.add_value(value_slot::LEXICAL_CONTAINER, docref::to_string(container->m_ref));
    }
    if (info->declAsContainer)
        doc.add_boolean_term(term::XCONTAINER);             /// \todo Does it really needed?

    // Get more terms/slots to attach
    auto type_flags = update_document_with_kind(info, doc);

    // Attach symbol type. Get aliased type for typedefs, get underlaid type for enums.
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
        const auto k = clang::kind_of(ct);
        if (k != CXType_Invalid && k != CXType_Unexposed)
        {
            auto type_str = to_string(clang::DCXString{clang_getTypeSpelling(ct)});
            if (!type_str.empty())
                doc.add_value(value_slot::TYPE, type_str);
            // Check some type properties
            if (clang_isPODType(ct))
            {
                doc.add_boolean_term(term::XPOD);
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

    if ((type_flags.m_redecl = bool(info->isRedeclaration)))
        doc.add_boolean_term(term::XREDECLARATION);

    if ((type_flags.m_implicit = bool(info->isImplicit)))
        doc.add_boolean_term(term::XIMPLICIT);

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
        clang_index_setClientContainer(
            info->declAsContainer
          , wrk->update_client_container(ref, name, qname)
          );
    }
}

void worker::on_declaration_reference(CXClientData client_data, const CXIdxEntityRefInfo* const info)
{
    auto* const wrk = static_cast<worker*>(client_data);
    Q_UNUSED(wrk);
    Q_UNUSED(info);
}

search_result::flags worker::update_document_with_kind(const CXIdxDeclInfo* info, document& doc)
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
            doc.add_boolean_term(term::XSTATIC);
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
                update_document_with_type_size(info, doc);
            }
            break;
        }
        case CXIdxEntity_CXXStaticVariable:
            doc.add_boolean_term(term::XSTATIC);
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
    }
}

void worker::update_document_with_base_classes(const CXIdxDeclInfo* info, document& doc)
{
    const auto* class_info = clang_index_getCXXClassDeclInfo(info);
    std::list<std::string> bases;
    if (class_info)
    {
        for (unsigned i = 0; i != class_info->numBases; ++i)
        {
            auto& base_info = class_info->bases[i]->base;
            auto name = string_cast<std::string>(base_info->name);
            assert("Unnamed base class?" && !name.empty());
            kDebug(DEBUG_AREA) << "found base class: " << name.c_str();
            kDebug(DEBUG_AREA) << "is-virtual-1: " << clang_isVirtualBase(base_info->cursor);
            kDebug(DEBUG_AREA) << "is-virtual-2: " << clang_isVirtualBase(class_info->bases[i]->cursor);
            doc.add_boolean_term(term::XBASE_CLASS + name);
            bases.emplace_back(std::move(name));
        }
    }
    // serialize bases list into a value slot
    std::stringstream ss{std::ios_base::out | std::ios_base::binary};
    boost::archive::binary_oarchive oa{ss};
    oa << bases;
    doc.add_value(value_slot::BASES, ss.str());
}

}}}                                                         // namespace details, index, kate
