/**
 * \file
 *
 * \brief Some helpers to deal w/ clang API
 *
 * \date Mon Nov 19 04:31:41 MSK 2012 -- Initial design
 */
/*
 * KateIncludeHelperPlugin is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * KateIncludeHelperPlugin is distributed in the hope that it will be useful, but
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
#include <map>

#define MAP_ENTRY(x) {x, #x}

namespace kate { namespace {

const std::map<CXCursorKind, QString> CURSOR_KIND_TO_STRING = {
    MAP_ENTRY(CXCursor_UnexposedDecl)
  , MAP_ENTRY(CXCursor_StructDecl)
  , MAP_ENTRY(CXCursor_UnionDecl)
  , MAP_ENTRY(CXCursor_ClassDecl)
  , MAP_ENTRY(CXCursor_EnumDecl)
  , MAP_ENTRY(CXCursor_FieldDecl)
  , MAP_ENTRY(CXCursor_EnumConstantDecl)
  , MAP_ENTRY(CXCursor_FunctionDecl)
  , MAP_ENTRY(CXCursor_VarDecl)
  , MAP_ENTRY(CXCursor_ParmDecl)
  , MAP_ENTRY(CXCursor_ObjCInterfaceDecl)
  , MAP_ENTRY(CXCursor_ObjCCategoryDecl)
  , MAP_ENTRY(CXCursor_ObjCProtocolDecl)
  , MAP_ENTRY(CXCursor_ObjCPropertyDecl)
  , MAP_ENTRY(CXCursor_ObjCIvarDecl)
  , MAP_ENTRY(CXCursor_ObjCInstanceMethodDecl)
  , MAP_ENTRY(CXCursor_ObjCClassMethodDecl)
  , MAP_ENTRY(CXCursor_ObjCImplementationDecl)
  , MAP_ENTRY(CXCursor_ObjCCategoryImplDecl)
  , MAP_ENTRY(CXCursor_TypedefDecl)
  , MAP_ENTRY(CXCursor_CXXMethod)
  , MAP_ENTRY(CXCursor_Namespace)
  , MAP_ENTRY(CXCursor_LinkageSpec)
  , MAP_ENTRY(CXCursor_Constructor)
  , MAP_ENTRY(CXCursor_Destructor)
  , MAP_ENTRY(CXCursor_ConversionFunction)
  , MAP_ENTRY(CXCursor_TemplateTypeParameter)
  , MAP_ENTRY(CXCursor_NonTypeTemplateParameter)
  , MAP_ENTRY(CXCursor_TemplateTemplateParameter)
  , MAP_ENTRY(CXCursor_FunctionTemplate)
  , MAP_ENTRY(CXCursor_ClassTemplate)
  , MAP_ENTRY(CXCursor_ClassTemplatePartialSpecialization)
  , MAP_ENTRY(CXCursor_NamespaceAlias)
  , MAP_ENTRY(CXCursor_UsingDirective)
  , MAP_ENTRY(CXCursor_UsingDeclaration)
  , MAP_ENTRY(CXCursor_TypeAliasDecl)
  , MAP_ENTRY(CXCursor_ObjCSynthesizeDecl)
  , MAP_ENTRY(CXCursor_ObjCDynamicDecl)
  , MAP_ENTRY(CXCursor_CXXAccessSpecifier)
  , MAP_ENTRY(CXCursor_ObjCSuperClassRef)
  , MAP_ENTRY(CXCursor_ObjCProtocolRef)
  , MAP_ENTRY(CXCursor_ObjCClassRef)
  , MAP_ENTRY(CXCursor_TypeRef)
  , MAP_ENTRY(CXCursor_CXXBaseSpecifier)
  , MAP_ENTRY(CXCursor_TemplateRef)
  , MAP_ENTRY(CXCursor_NamespaceRef)
  , MAP_ENTRY(CXCursor_MemberRef)
  , MAP_ENTRY(CXCursor_LabelRef)
  , MAP_ENTRY(CXCursor_OverloadedDeclRef)
  , MAP_ENTRY(CXCursor_VariableRef)
  , MAP_ENTRY(CXCursor_InvalidFile)
  , MAP_ENTRY(CXCursor_NoDeclFound)
  , MAP_ENTRY(CXCursor_NotImplemented)
  , MAP_ENTRY(CXCursor_InvalidCode)
  , MAP_ENTRY(CXCursor_UnexposedExpr)
  , MAP_ENTRY(CXCursor_DeclRefExpr)
  , MAP_ENTRY(CXCursor_MemberRefExpr)
  , MAP_ENTRY(CXCursor_CallExpr)
  , MAP_ENTRY(CXCursor_ObjCMessageExpr)
  , MAP_ENTRY(CXCursor_BlockExpr)
  , MAP_ENTRY(CXCursor_IntegerLiteral)
  , MAP_ENTRY(CXCursor_FloatingLiteral)
  , MAP_ENTRY(CXCursor_ImaginaryLiteral)
  , MAP_ENTRY(CXCursor_StringLiteral)
  , MAP_ENTRY(CXCursor_CharacterLiteral)
  , MAP_ENTRY(CXCursor_ParenExpr)
  , MAP_ENTRY(CXCursor_UnaryOperator)
  , MAP_ENTRY(CXCursor_ArraySubscriptExpr)
  , MAP_ENTRY(CXCursor_BinaryOperator)
  , MAP_ENTRY(CXCursor_CompoundAssignOperator)
  , MAP_ENTRY(CXCursor_ConditionalOperator)
  , MAP_ENTRY(CXCursor_CStyleCastExpr)
  , MAP_ENTRY(CXCursor_CompoundLiteralExpr)
  , MAP_ENTRY(CXCursor_InitListExpr)
  , MAP_ENTRY(CXCursor_AddrLabelExpr)
  , MAP_ENTRY(CXCursor_StmtExpr)
  , MAP_ENTRY(CXCursor_GenericSelectionExpr)
  , MAP_ENTRY(CXCursor_GNUNullExpr)
  , MAP_ENTRY(CXCursor_CXXStaticCastExpr)
  , MAP_ENTRY(CXCursor_CXXDynamicCastExpr)
  , MAP_ENTRY(CXCursor_CXXReinterpretCastExpr)
  , MAP_ENTRY(CXCursor_CXXConstCastExpr)
  , MAP_ENTRY(CXCursor_CXXFunctionalCastExpr)
  , MAP_ENTRY(CXCursor_CXXTypeidExpr)
  , MAP_ENTRY(CXCursor_CXXBoolLiteralExpr)
  , MAP_ENTRY(CXCursor_CXXNullPtrLiteralExpr)
  , MAP_ENTRY(CXCursor_CXXThisExpr)
  , MAP_ENTRY(CXCursor_CXXThrowExpr)
  , MAP_ENTRY(CXCursor_CXXNewExpr)
  , MAP_ENTRY(CXCursor_CXXDeleteExpr)
  , MAP_ENTRY(CXCursor_UnaryExpr)
  , MAP_ENTRY(CXCursor_ObjCStringLiteral)
  , MAP_ENTRY(CXCursor_ObjCEncodeExpr)
  , MAP_ENTRY(CXCursor_ObjCSelectorExpr)
  , MAP_ENTRY(CXCursor_ObjCProtocolExpr)
  , MAP_ENTRY(CXCursor_ObjCBridgedCastExpr)
  , MAP_ENTRY(CXCursor_PackExpansionExpr)
  , MAP_ENTRY(CXCursor_SizeOfPackExpr)
  , MAP_ENTRY(CXCursor_LambdaExpr)
  , MAP_ENTRY(CXCursor_ObjCBoolLiteralExpr)
  , MAP_ENTRY(CXCursor_UnexposedStmt)
  , MAP_ENTRY(CXCursor_LabelStmt)
  , MAP_ENTRY(CXCursor_CompoundStmt)
  , MAP_ENTRY(CXCursor_CaseStmt)
  , MAP_ENTRY(CXCursor_DefaultStmt)
  , MAP_ENTRY(CXCursor_IfStmt)
  , MAP_ENTRY(CXCursor_SwitchStmt)
  , MAP_ENTRY(CXCursor_WhileStmt)
  , MAP_ENTRY(CXCursor_DoStmt)
  , MAP_ENTRY(CXCursor_ForStmt)
  , MAP_ENTRY(CXCursor_GotoStmt)
  , MAP_ENTRY(CXCursor_IndirectGotoStmt)
  , MAP_ENTRY(CXCursor_ContinueStmt)
  , MAP_ENTRY(CXCursor_BreakStmt)
  , MAP_ENTRY(CXCursor_ReturnStmt)
  , MAP_ENTRY(CXCursor_AsmStmt)
  , MAP_ENTRY(CXCursor_ObjCAtTryStmt)
  , MAP_ENTRY(CXCursor_ObjCAtCatchStmt)
  , MAP_ENTRY(CXCursor_ObjCAtFinallyStmt)
  , MAP_ENTRY(CXCursor_ObjCAtThrowStmt)
  , MAP_ENTRY(CXCursor_ObjCAtSynchronizedStmt)
  , MAP_ENTRY(CXCursor_ObjCAutoreleasePoolStmt)
  , MAP_ENTRY(CXCursor_ObjCForCollectionStmt)
  , MAP_ENTRY(CXCursor_CXXCatchStmt)
  , MAP_ENTRY(CXCursor_CXXTryStmt)
  , MAP_ENTRY(CXCursor_CXXForRangeStmt)
  , MAP_ENTRY(CXCursor_SEHTryStmt)
  , MAP_ENTRY(CXCursor_SEHExceptStmt)
  , MAP_ENTRY(CXCursor_SEHFinallyStmt)
  , MAP_ENTRY(CXCursor_NullStmt)
  , MAP_ENTRY(CXCursor_DeclStmt)
  , MAP_ENTRY(CXCursor_TranslationUnit)
  , MAP_ENTRY(CXCursor_UnexposedAttr)
  , MAP_ENTRY(CXCursor_IBActionAttr)
  , MAP_ENTRY(CXCursor_IBOutletAttr)
  , MAP_ENTRY(CXCursor_IBOutletCollectionAttr)
  , MAP_ENTRY(CXCursor_CXXFinalAttr)
  , MAP_ENTRY(CXCursor_CXXOverrideAttr)
  , MAP_ENTRY(CXCursor_AnnotateAttr)
  , MAP_ENTRY(CXCursor_AsmLabelAttr)
  , MAP_ENTRY(CXCursor_PreprocessingDirective)
  , MAP_ENTRY(CXCursor_MacroDefinition)
  , MAP_ENTRY(CXCursor_MacroExpansion)
  , MAP_ENTRY(CXCursor_InclusionDirective)
};

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
}                                                           // anonymous namespace

/// Get a human readable string of \c CXCursorKind
QString toString(const CXCursorKind v) try
{
    return CURSOR_KIND_TO_STRING.at(v);
}
catch (const std::exception&)
{
    return QString::number(v);
}
/// Get a human readable string of \c CXCompletionChunkKind
QString toString(const CXCompletionChunkKind v) try
{
    return CHUNK_KIND_TO_STRING.at(v);
}
catch (const std::exception&)
{
    return QString::number(v);
}
}                                                           // namespace kate
#undef MAP_ENTRY
// kate: hl C++11/Qt4;
