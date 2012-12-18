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

// Standard includes
#include <QtCore/QRegExp>
#include <cassert>
#include <map>
#include <vector>

namespace kate { namespace {
const QString DEPRECATED_STR = "DEPRECATED";
}                                                           // anonymous namespace

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
                    result = m_text;
                    break;
                case KTextEditor::CodeCompletionModel::Prefix:
                {
                    auto prefix = renderPlaceholders(m_before);
                    if (prefix.isEmpty())
                    {
                        switch (m_kind)
                        {
                            case CXCursor_TypedefDecl:   prefix = QLatin1String("typedef");         break;
                            case CXCursor_ClassDecl:     prefix = QLatin1String("class");           break;
                            case CXCursor_ClassTemplate: prefix = QLatin1String("template class");  break;
                            case CXCursor_StructDecl:    prefix = QLatin1String("struct");          break;
                            case CXCursor_EnumDecl:      prefix = QLatin1String("enum");            break;
                            case CXCursor_Namespace:     prefix = QLatin1String("namespace");       break;
                            case CXCursor_UnionDecl:     prefix = QLatin1String("union");           break;
                            default: break;
                        }
                        result = prefix;
                    }
                    else result = sanitizePrefix(std::move(prefix));
                    break;
                }
                case KTextEditor::CodeCompletionModel::Postfix:
                    if (m_deprecated)
                        result = DEPRECATED_STR;
                    break;
                case KTextEditor::CodeCompletionModel::Arguments:
                    result = sanitizeParams(renderPlaceholders(m_after));
                    break;
                // NOTE This would just merge `scope` text w/ name and finally
                // everything would look ugly... Anyway we have groups to join
                // completion items by parent/scope and it looks better than this feature...
                case KTextEditor::CodeCompletionModel::Scope:
                case KTextEditor::CodeCompletionModel::Icon:
                default:
                    break;
            }
            break;
        default:
#if 0
            kDebug() << "Role" << role << "requested for" << index;
#endif
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

namespace {
/**
 * Just a static bunch of plain replacements for <em>some well known cases</em>
 */
std::vector<std::pair<QLatin1String, QLatin1String>> SIMPLE_REPLACEMENTS = {
    {QLatin1String("std::basic_string<char>"), QLatin1String("std::string")}
  , {QLatin1String("std::basic_string<wchar_t>"), QLatin1String("std::wstring")}

  , {QLatin1String("basic_streambuf<char, std::char_traits<char> >"), QLatin1String("std::streambuf")}
  , {QLatin1String("basic_ostream<char, std::char_traits<char> >"), QLatin1String("std::ostream")}
  , {QLatin1String("basic_istream<char, std::char_traits<char> >"), QLatin1String("std::istream")}
  , {
        QLatin1String("std::istreambuf_iterator<char, std::char_traits<char> >")
      , QLatin1String("std::istreambuf_iterator<char>")
    }

  , {QLatin1String("basic_streambuf<wchar_t, std::char_traits<wchar_t> >"), QLatin1String("std::wstreambuf")}
  , {QLatin1String("basic_ostream<wchar_t, std::char_traits<wchar_t> >"), QLatin1String("std::wostream")}
  , {QLatin1String("basic_istream<wchar_t, std::char_traits<wchar_t> >"), QLatin1String("std::wistream")}
};
/**
 * \todo Regular expressions are suck! They aren't suitable to parse C++... definitely!
 * But for getting quick results here is some set of beautifiers for standard types.
 * Unfortunately it's matter of luck if they would work... For example, a simple type like:
 * \code
 *  std::map<std::pair<char, short>, int>
 * \endcode
 * will not be sanitized... moreover, it damn hard to write a correct regex for even this simple case.
 */
std::vector<std::pair<QRegExp, QString>> REGEX_REPLACEMENTS = {
    {
        QRegExp(R"--(std::(deque|list|vector)<(.*), std::allocator<\2> >)--")
      , QString(R"--(std::\1<\2>)--")
    }
  , {
        QRegExp(R"--(std::(multimap|map)<(.*), (.*), std::less<\2>, std::allocator<std::pair<const \2, \3> > >)--")
      , QString(R"--(std::\1<\2, \3>)--")
    }
  , {
        QRegExp(R"--(std::(multiset|set)<(.*), std::less<\2>, std::allocator<\2> >)--")
      , QString(R"--(std::\1<\2>)--")
    }
  , {
        // Stip tail of boost::variant default template types
        QRegExp(R"--((, boost::detail::variant::void_)*>)--")
      , QString(">")
    }
  , {
        // Stip tail of boost::mpl containers default template types
        QRegExp(R"--((, mpl_::na)*>)--")
      , QString(">")
    }
};

const QLatin1String DBL_TEMPLATE_CLOSE_BEFORE(" >");
const QString DBL_TEMPLATE_CLOSE_AFTER(">");

inline QString sanitizeTemplateCloseBrakets(QString&& text)
{
    for (
        auto pos = text.indexOf(DBL_TEMPLATE_CLOSE_BEFORE)
      ; pos != -1
      ; pos = text.indexOf(DBL_TEMPLATE_CLOSE_BEFORE, pos)
      ) text.replace(pos, 3, DBL_TEMPLATE_CLOSE_AFTER);
    return text;
}

inline QString sanitizeCompletionText(QString&& text)
{
    for (const auto& p : SIMPLE_REPLACEMENTS)
        text.replace(p.first, p.second);
    for (const auto& p : REGEX_REPLACEMENTS)
        text.replace(p.first, p.second);
    return text;
}
}                                                           // anonymous namespace

QString ClangCodeCompletionItem::sanitizeParams(QString&& params) const
{
    return sanitizeTemplateCloseBrakets(sanitizeCompletionText(std::move(params)));
}

QString ClangCodeCompletionItem::sanitizePrefix(QString&& prefix) const
{
    prefix = sanitizeCompletionText(std::move(prefix));
    prefix.replace(QLatin1String(" &"), QLatin1String("&"));
    prefix.replace(QLatin1String(" *"), QLatin1String("*"));
    return sanitizeTemplateCloseBrakets(std::move(prefix));
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
