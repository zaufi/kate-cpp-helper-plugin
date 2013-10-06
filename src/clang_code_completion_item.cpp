/**
 * \file
 *
 * \brief Class \c kate::ClangCodeCompletionItem (implementation)
 *
 * \date Mon Nov 19 04:21:06 MSK 2012 -- Initial design
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
#include <src/clang_code_completion_item.h>
#include <src/clang/to_string.h>
#include <src/sanitize_snippet.h>

// Standard includes
#include <KDE/KIcon>
#include <cassert>
#include <map>

namespace kate { namespace {
const QString DEPRECATED_STR = "DEPRECATED";
}                                                           // anonymous namespace

/**
 * Produce a data siutable for view
 */
QVariant ClangCodeCompletionItem::data(
    const QModelIndex& index
  , const int role
  , const bool use_prefix_column
  ) const
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
        case Qt::DecorationRole:
            if (!use_prefix_column && index.column() == KTextEditor::CodeCompletionModel::Icon)
                result = icon();
            break;
        case Qt::DisplayRole:
            switch (index.column())
            {
                case KTextEditor::CodeCompletionModel::Prefix:
                    if (use_prefix_column)
                        result = renderPrefix();
                    break;
                case KTextEditor::CodeCompletionModel::Name:
                    if (use_prefix_column)
                        result = m_text;
                    else
                        result = QString(renderPrefix() + " " + m_text);
                    break;
                case KTextEditor::CodeCompletionModel::Postfix:
                    if (m_deprecated)
                        result = DEPRECATED_STR;
                    break;
                case KTextEditor::CodeCompletionModel::Arguments:
                    result = renderPlaceholders(m_after);
                    break;
                // NOTE This would just merge `scope` text w/ name and finally
                // everything would look ugly... Anyway we have groups to join
                // completion items by parent/scope and it looks better than this feature...
                case KTextEditor::CodeCompletionModel::Scope:
                    break;
                default:
                    break;
            }
            break;
        default:
#if 0
            kDebug(DEBUG_AREA) << "Role" << role << "requested for" << index;
#endif
            break;
    }
    return result;
}

QString ClangCodeCompletionItem::renderPrefix() const
{
    auto prefix = renderPlaceholders(m_before);
    if (prefix.isEmpty())
    {
        switch (m_kind)
        {
            case CXCursor_TypedefDecl:      prefix = QLatin1String("typedef");        break;
            case CXCursor_ClassDecl:        prefix = QLatin1String("class");          break;
            case CXCursor_ClassTemplate:    prefix = QLatin1String("template class"); break;
            case CXCursor_StructDecl:       prefix = QLatin1String("struct");         break;
            case CXCursor_EnumDecl:         prefix = QLatin1String("enum");           break;
            case CXCursor_Namespace:        prefix = QLatin1String("namespace");      break;
            case CXCursor_UnionDecl:        prefix = QLatin1String("union");          break;
            case CXCursor_MacroDefinition:  prefix = QLatin1String("macro");          break;
            default: break;
        }
    }
    return prefix;
}

/// \todo This look ugly... need to invent smth clever
QString ClangCodeCompletionItem::renderPlaceholders(const QString& source) const
{
    QString result = source;
    unsigned i = 0;
    if (m_optional_placeholders_pos != NO_OPTIONAL_PLACEHOLDERS)
    {
        // Ok be ready for optional parameters
        for (const auto& p : m_placeholders)
        {
            const QString fmt = '%' + QString::number(++i) + '%';
            auto pos = result.indexOf(fmt);
            auto text = p;
            if (i == unsigned(m_optional_placeholders_pos))
                text.prepend('[');
            if (i == unsigned(m_placeholders.size()))
                text.append(']');
            if (pos != -1)
                result = result.replace(pos, fmt.length(), text);
        }
    }
    else
    {
        // No optional parameters
        for (const auto& p : m_placeholders)
        {
            const QString fmt = '%' + QString::number(++i) + '%';
            auto pos = result.indexOf(fmt);
            if (pos != -1)
                result = result.replace(pos, fmt.length(), p);
        }
    }
    return result;
}

namespace {
/**
 *  NoProperty
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
 */
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
  , {CXCursor_CXXMethod, KTextEditor::CodeCompletionModel::Function}
  , {CXCursor_ConversionFunction, KTextEditor::CodeCompletionModel::Function}
  , {CXCursor_Destructor, KTextEditor::CodeCompletionModel::Function}
  , {CXCursor_FunctionDecl, KTextEditor::CodeCompletionModel::Function}
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

KTextEditor::CodeCompletionModel::CompletionProperty ClangCodeCompletionItem::completionProperty() const
{
    auto it = CURSOR_PREPERTY_MAP.find(m_kind);
    if (it != std::end(CURSOR_PREPERTY_MAP))
        return it->second;
    return KTextEditor::CodeCompletionModel::NoProperty;
}

QPair<QString, int> ClangCodeCompletionItem::executeCompletion() const
{
    auto result = m_text;
    int pos = -1;
    switch (m_kind)
    {
        case CXCursor_ClassTemplate:
            result += "<>";
            pos = -2;
            break;
        case CXCursor_CXXMethod:
        case CXCursor_ConversionFunction:
        case CXCursor_Destructor:
        case CXCursor_FunctionDecl:
        case CXCursor_FunctionTemplate:
        case CXCursor_MemberRef:
        case CXCursor_OverloadedDeclRef:
            result += "()";
            if (!m_placeholders.isEmpty())
                pos = -2;                                   // Will move cursor only if function requires params
        default:
            break;
    }
    return qMakePair(result, result.length() + pos + 1);
}

auto ClangCodeCompletionItem::getCompletionTemplate() const -> CompletionTemplateData
{
    QString tpl = m_text + m_after;
    bool is_function = false;
    switch (m_kind)
    {
        case CXCursor_CXXMethod:
        case CXCursor_Constructor:
        case CXCursor_Destructor:
        case CXCursor_FunctionDecl:
        case CXCursor_FunctionTemplate:
        case CXCursor_OverloadedDeclRef:
        {
            is_function = true;
            // Strip informational text: like `const' or `volatile' from function-members
            auto pos = tpl.lastIndexOf(')');
            if (pos != -1)
                tpl.remove(pos + 1, tpl.length());
            break;
        }
        default:
            break;
    }
    QMap<QString, QString> values;
    unsigned i = 0;
    for (const auto& p : m_placeholders)
    {
        const QString fmt = '%' + QString::number(++i) + '%';
        auto pos = tpl.indexOf(fmt);
        if (pos != -1)
        {
            QString arg = QLatin1String("arg") + QString::number(i);
            QString placeholder = QLatin1String("${") + arg + '}';
            tpl = tpl.replace(
                pos
              , fmt.length()
              , placeholder
              );
            values[arg] = p;
        }
    }
#if 0
    tpl += QLatin1String("%{cursor}");
#endif
    return {tpl, values, is_function};
}

namespace {
const std::map<
    CXCursorKind
  , const char*
  > ICONS_MAP = {
    // Namespace
    {CXCursor_Namespace, "code-context"}
  , {CXCursor_NamespaceRef, "code-context"}
    // Class
  , {CXCursor_ClassDecl, "code-class"}
  , {CXCursor_ClassTemplate, "code-class"}
    // Struct
  , {CXCursor_StructDecl, "code-class"}
    // Union
  , {CXCursor_UnionDecl, "code-class"}
    // Enum
  , {CXCursor_EnumDecl, "code-class"}
    // Function
  , {CXCursor_CXXMethod, "code-function"}
  , {CXCursor_ConversionFunction, "code-function"}
  , {CXCursor_Destructor, "code-function"}
  , {CXCursor_FunctionDecl, "code-function"}
  , {CXCursor_FunctionTemplate, "code-function"}
  , {CXCursor_MemberRef, "code-function"}
  , {CXCursor_OverloadedDeclRef, "code-function"}
    // Variable
  , {CXCursor_VarDecl, "code-variable"}
  , {CXCursor_VariableRef, "code-variable"}
    // Data-member
  , {CXCursor_FieldDecl, "code-variable"}
    // TypeAlias
  , {CXCursor_TypedefDecl, "code-typedef"}
  , {CXCursor_TypeAliasDecl, "code-typedef"}
  , {CXCursor_TypeRef, "code-typedef"}
    // Template
  , {CXCursor_TemplateRef, "code-block"}
};
}                                                           // anonymous namespace

QVariant ClangCodeCompletionItem::icon() const
{
    auto it = ICONS_MAP.find(m_kind);
    if (it != std::end(ICONS_MAP))
        return KIcon(it->second);
    kDebug(DEBUG_AREA) << "Item kind has no icon defined: " << clang::toString(m_kind);
    return QVariant();
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
