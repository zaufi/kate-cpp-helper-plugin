/**
 * \file
 *
 * \brief Some helpers to deal w/ clang API
 *
 * \date Mon Nov 19 04:31:41 MSK 2012 -- Initial design
 */
/*
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
#include <src/clang_utils.h>

// Standard includes
#include <cassert>
#include <map>

#define MAP_ENTRY(x) {x, #x}

namespace kate { namespace {

const std::map<CXCompletionChunkKind, QString> CHUNK_KIND_TO_STRING = {
    MAP_ENTRY(CXCompletionChunk_Optional)
  , MAP_ENTRY(CXCompletionChunk_TypedText)
  , MAP_ENTRY(CXCompletionChunk_Text)
  , MAP_ENTRY(CXCompletionChunk_Placeholder)
  , MAP_ENTRY(CXCompletionChunk_Informative)
  , MAP_ENTRY(CXCompletionChunk_CurrentParameter)
  , MAP_ENTRY(CXCompletionChunk_LeftParen)
  , MAP_ENTRY(CXCompletionChunk_RightParen)
  , MAP_ENTRY(CXCompletionChunk_LeftBracket)
  , MAP_ENTRY(CXCompletionChunk_RightBracket)
  , MAP_ENTRY(CXCompletionChunk_LeftBrace)
  , MAP_ENTRY(CXCompletionChunk_RightBrace)
  , MAP_ENTRY(CXCompletionChunk_LeftAngle)
  , MAP_ENTRY(CXCompletionChunk_RightAngle)
  , MAP_ENTRY(CXCompletionChunk_Comma)
  , MAP_ENTRY(CXCompletionChunk_ResultType)
  , MAP_ENTRY(CXCompletionChunk_Colon)
  , MAP_ENTRY(CXCompletionChunk_SemiColon)
  , MAP_ENTRY(CXCompletionChunk_Equal)
  , MAP_ENTRY(CXCompletionChunk_HorizontalSpace)
  , MAP_ENTRY(CXCompletionChunk_VerticalSpace)
};

const std::map<CXIdxEntityKind, QString> ENTITY_KIND_TO_STRING = {
    {CXIdxEntity_Unexposed, "<<UNEXPOSED>>"}
  , {CXIdxEntity_Typedef, "typedef"}
  , {CXIdxEntity_Function, "function"}
  , {CXIdxEntity_Variable, "variable"}
  , {CXIdxEntity_Field, "field"}
  , {CXIdxEntity_EnumConstant, "enumerator"}
  , {CXIdxEntity_ObjCClass, "objc-class"}
  , {CXIdxEntity_ObjCProtocol, "objc-protocol"}
  , {CXIdxEntity_ObjCCategory, "objc-category"}
  , {CXIdxEntity_ObjCInstanceMethod, "objc-instance-method"}
  , {CXIdxEntity_ObjCClassMethod, "objc-class-method"}
  , {CXIdxEntity_ObjCProperty, "objc-property"}
  , {CXIdxEntity_ObjCIvar, "objc-ivar"}
  , {CXIdxEntity_Enum, "enum"}
  , {CXIdxEntity_Struct, "struct"}
  , {CXIdxEntity_Union, "union"}
  , {CXIdxEntity_CXXClass, "c++-class"}
  , {CXIdxEntity_CXXNamespace, "namespace"}
  , {CXIdxEntity_CXXNamespaceAlias, "namespace-alias"}
  , {CXIdxEntity_CXXStaticVariable, "c++-static-var"}
  , {CXIdxEntity_CXXStaticMethod, "c++-static-method"}
  , {CXIdxEntity_CXXInstanceMethod, "c++-instance-method"}
  , {CXIdxEntity_CXXConstructor, "constructor"}
  , {CXIdxEntity_CXXDestructor, "destructor"}
  , {CXIdxEntity_CXXConversionFunction, "conversion-func"}
  , {CXIdxEntity_CXXTypeAlias, "type-alias"}
  , {CXIdxEntity_CXXInterface, "c++-__interface"}
};

const std::map<CXIdxEntityCXXTemplateKind, QString> ENTITY_CXX_TEMPLATE_KIND_TO_STRING = {
    {CXIdxEntity_NonTemplate, QString()}
  , {CXIdxEntity_Template, "-template"}
  , {CXIdxEntity_TemplatePartialSpecialization, "-template-partial-spec"}
  , {CXIdxEntity_TemplateSpecialization, "-template-spec"}
};

}                                                           // anonymous namespace

/// Get a human readable string of \c CXCompletionChunkKind
QString toString(const CXCompletionChunkKind kind) try
{
#if 0
    /// \todo This expected to work but it doesn't due an undefined function ;(
    DCXString str = clang_getCompletionChunkKindSpelling(v);
    return QString(clang_getCString(str));
#endif
    return CHUNK_KIND_TO_STRING.at(kind);
}
catch (const std::exception&)
{
    return QString::number(kind);
}

QString toString(const CXIdxEntityKind kind) try
{
    return ENTITY_KIND_TO_STRING.at(kind);
}
catch (const std::exception&)
{
    return QString::number(kind);
}

QString toString(const CXIdxEntityCXXTemplateKind kind) try
{
    return ENTITY_CXX_TEMPLATE_KIND_TO_STRING.at(kind);
}
catch (const std::exception&)
{
    assert(!"Garbage entity kind");
    return QString::number(kind);
}


location::location(const CXIdxLoc loc)
{
    CXIdxClientFile file;
    unsigned line;
    unsigned column;
    unsigned offset;
    clang_indexLoc_getFileLocation(loc, &file, nullptr, &line, &column, &offset);
    if (line == 0)
        throw exception::invalid();
    if (file == nullptr)
        throw exception::invalid("No index file has attached to a source location");
    DCXString filename = {clang_getFileName(static_cast<CXFile>(file))};
    m_file = clang_getCString(filename);
    m_line = line;
    m_column = column;
    m_offset = offset;
}

location::location(const CXSourceLocation loc)
{
    CXFile file;
    unsigned line;
    unsigned column;
    unsigned offset;
    clang_getSpellingLocation(loc, &file, &line, &column, &offset);
    if (file == nullptr)
        throw exception::invalid("No file has attached to a source location");
    DCXString filename = {clang_getFileName(static_cast<CXFile>(file))};
    m_file = clang_getCString(filename);
    m_line = line;
    m_column = column;
    m_offset = offset;
}

}                                                           // namespace kate
#undef MAP_ENTRY
// kate: hl C++11/Qt4;
