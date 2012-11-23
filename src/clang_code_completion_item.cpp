/**
 * \file
 *
 * \brief Class \c kate::ClangCodeCompletionItem (implementation)
 *
 * \date Mon Nov 19 04:21:06 MSK 2012 -- Initial design
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
#include <src/clang_code_completion_item.h>

// Standard includes
#include <KTextEditor/CodeCompletionModel>
#include <cassert>
#include <map>

namespace kate {
/**
 * Produce a data siutable for view
 */
QVariant ClangCodeCompletionItem::data(const QModelIndex& index, const int role) const
{
    assert("Sanity check" && index.isValid());
    //
    QVariant result;
    switch (role)
    {
#if 0
        // WARNING Assigning match quality doesn't looks (literally) a good idea!
        // It's really look ugly...
        case KTextEditor::CodeCompletionModel::MatchQuality:
            result = int(100u - m_priority) * 10;
            break;
#endif
        case KTextEditor::CodeCompletionModel::CompletionRole:
            result = completionProperty();
            break;
        case Qt::DisplayRole:
            switch (index.column())
            {
                case KTextEditor::CodeCompletionModel::Name:
                    // Ok put placeholders into 
                    result = m_text;
                    break;
                case KTextEditor::CodeCompletionModel::Prefix:
                    result = renderPlaceholders(m_before);
                    break;
                case KTextEditor::CodeCompletionModel::Postfix:
                    break;
                case KTextEditor::CodeCompletionModel::Arguments:
                    result = renderPlaceholders(m_after);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return result;
}

/// \todo This look ugly... need to invent smth clever
QString ClangCodeCompletionItem::renderPlaceholders(const QString& source) const
{
    QString result = source;
    unsigned i = 0;
    Q_FOREACH(const QString& p, m_placeholders)
    {
        ++i;
        QString num = QString::number(i);
        QString fmt = '%' + num + '%';
        if (result.contains(fmt))
            result = result.replace(fmt, p);
    }
    return result;
}

namespace {
const std::map<
    CXCursorKind
  , KTextEditor::CodeCompletionModel::CompletionProperty
  > CURSOR_PREPERTY_MAP = {
    // Namespace
    {CXCursor_Namespace, KTextEditor::CodeCompletionModel::Namespace}
  , {CXCursor_NamespaceRef, KTextEditor::CodeCompletionModel::Namespace}
    // Class
  , {CXCursor_ClassDecl, KTextEditor::CodeCompletionModel::Class}
  , {CXCursor_ClassTemplate, KTextEditor::CodeCompletionModel::Class}
    // Struct
  , {CXCursor_StructDecl, KTextEditor::CodeCompletionModel::Struct}
    // Union
  , {CXCursor_UnionDecl, KTextEditor::CodeCompletionModel::Union}
    // Enum
  , {CXCursor_EnumDecl, KTextEditor::CodeCompletionModel::Enum}
    // Function
  , {CXCursor_FunctionDecl, KTextEditor::CodeCompletionModel::Function}
  , {CXCursor_CXXMethod, KTextEditor::CodeCompletionModel::Function}
  , {CXCursor_Destructor, KTextEditor::CodeCompletionModel::Function}
  , {CXCursor_ConversionFunction, KTextEditor::CodeCompletionModel::Function}
  , {CXCursor_FunctionTemplate, KTextEditor::CodeCompletionModel::Function}
  , {CXCursor_MemberRef, KTextEditor::CodeCompletionModel::Function}
  , {CXCursor_OverloadedDeclRef, KTextEditor::CodeCompletionModel::Function}
    // Variable
  , {CXCursor_VarDecl, KTextEditor::CodeCompletionModel::Variable}
  , {CXCursor_VariableRef, KTextEditor::CodeCompletionModel::Variable}
    // TypeAlias
  , {CXCursor_TypedefDecl, KTextEditor::CodeCompletionModel::TypeAlias}
  , {CXCursor_TypeAliasDecl, KTextEditor::CodeCompletionModel::TypeAlias}
  , {CXCursor_TypeRef, KTextEditor::CodeCompletionModel::TypeAlias}
    // Template
  , {CXCursor_TemplateRef, KTextEditor::CodeCompletionModel::Template}
};
}                                                           // anonymous namespace


/**
 *  NoProperty
 *  FirstProperty
 *  Public
 *  Protected
 *  Private
 *  Static
 *  Const
 *  Namespace
 *  Class
 *  Struct
 *  Union
 *  Function
 *  Variable
 *  Enum
 *  Template
 *  TypeAlias
 *  Virtual
 *  Override
 *  Inline
 *  Friend
 *  Signal
 *  Slot
 *  LocalScope
 *  NamespaceScope
 *  GlobalScope
 *  LastProperty
 */
KTextEditor::CodeCompletionModel::CompletionProperty ClangCodeCompletionItem::completionProperty() const
{
    auto it = CURSOR_PREPERTY_MAP.find(m_kind);
    if (it != std::end(CURSOR_PREPERTY_MAP))
        return it->second;
    return KTextEditor::CodeCompletionModel::NoProperty;
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
