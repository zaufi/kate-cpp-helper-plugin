/**
 * \file
 *
 * \brief Class \c kate::clang::to_string (implementation)
 *
 * \date Sun Oct  6 23:31:48 MSK 2013 -- Initial design
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
#include <src/clang/to_string.h>

// Standard includes
#include <boost/lexical_cast.hpp>
#include <cassert>
#include <map>

namespace kate { namespace clang { namespace {

const std::map<CXCompletionChunkKind, const char*> CHUNK_KIND_TO_STRING = {
    {CXCompletionChunk_Optional, "Optional"}
  , {CXCompletionChunk_TypedText, "TypedText"}
  , {CXCompletionChunk_Text, "Text"}
  , {CXCompletionChunk_Placeholder, "Placeholder"}
  , {CXCompletionChunk_Informative, "Informative"}
  , {CXCompletionChunk_CurrentParameter, "CurrentParameter"}
  , {CXCompletionChunk_LeftParen, "LeftParen"}
  , {CXCompletionChunk_RightParen, "RightParen"}
  , {CXCompletionChunk_LeftBracket, "LeftBracket"}
  , {CXCompletionChunk_RightBracket, "RightBracket"}
  , {CXCompletionChunk_LeftBrace, "LeftBrace"}
  , {CXCompletionChunk_RightBrace, "RightBrace"}
  , {CXCompletionChunk_LeftAngle, "LeftAngle"}
  , {CXCompletionChunk_RightAngle, "RightAngle"}
  , {CXCompletionChunk_Comma, "Comma"}
  , {CXCompletionChunk_ResultType, "ResultType"}
  , {CXCompletionChunk_Colon, "Colon"}
  , {CXCompletionChunk_SemiColon, "SemiColon"}
  , {CXCompletionChunk_Equal, "Equal"}
  , {CXCompletionChunk_HorizontalSpace, "HorizontalSpace"}
  , {CXCompletionChunk_VerticalSpace, "VerticalSpace"}
};

const std::map<CXIdxEntityKind, const char*> ENTITY_KIND_TO_STRING = {
    {CXIdxEntity_Unexposed, "<<UNEXPOSED>>"}
  , {CXIdxEntity_Typedef, "typedef"}
  , {CXIdxEntity_Function, "function"}
  , {CXIdxEntity_Variable, "variable"}
  , {CXIdxEntity_Field, "field"}
  , {CXIdxEntity_EnumConstant, "enum-const"}
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
  , {CXIdxEntity_CXXInterface, "c++-interface"}
};

const std::map<CXIdxEntityCXXTemplateKind, const char*> ENTITY_CXX_TEMPLATE_KIND_TO_STRING = {
    {CXIdxEntity_NonTemplate, ""}
  , {CXIdxEntity_Template, "-template"}
  , {CXIdxEntity_TemplatePartialSpecialization, "-template-partial-spec"}
  , {CXIdxEntity_TemplateSpecialization, "-template-spec"}
};

const std::map<CXLinkageKind, const char*> LINKAGE_KIND_TO_STRING = {
    {CXLinkage_Invalid, "<invalid>"}
  , {CXLinkage_NoLinkage, ""}
  , {CXLinkage_Internal, "int"}
  , {CXLinkage_UniqueExternal, "uniq-ext"}
  , {CXLinkage_External, "ext"}
};
}                                                           // anonymous namespace

/// Get a human readable string of \c CXCompletionChunkKind
QString toString(const CXCompletionChunkKind kind) try
{
    return CHUNK_KIND_TO_STRING.at(kind);
}
catch (const std::exception&)
{
    return QString::number(kind);
}

std::string to_string(const CXCompletionChunkKind kind) try
{
    return CHUNK_KIND_TO_STRING.at(kind);
}
catch (const std::exception&)
{
    return boost::lexical_cast<std::string>(kind);
}

QString toString(const CXIdxEntityKind kind) try
{
    return ENTITY_KIND_TO_STRING.at(kind);
}
catch (const std::exception&)
{
    return QString::number(kind);
}

std::string to_string(const CXIdxEntityKind kind) try
{
    return ENTITY_KIND_TO_STRING.at(kind);
}
catch (const std::exception&)
{
    return boost::lexical_cast<std::string>(kind);
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

std::string to_string(const CXIdxEntityCXXTemplateKind kind) try
{
    return ENTITY_CXX_TEMPLATE_KIND_TO_STRING.at(kind);
}
catch (const std::exception&)
{
    assert(!"Garbage entity kind");
    return boost::lexical_cast<std::string>(kind);
}

QString toString(const CXLinkageKind kind) try
{
    return LINKAGE_KIND_TO_STRING.at(kind);
}
catch (const std::exception&)
{
    assert(!"Garbage entity kind");
    return QString::number(kind);
}

std::string to_string(const CXLinkageKind kind) try
{
    return LINKAGE_KIND_TO_STRING.at(kind);
}
catch (const std::exception&)
{
    assert(!"Garbage entity kind");
    return boost::lexical_cast<std::string>(kind);
}

}}                                                          // namespace clang, kate
