/**
 * \file
 *
 * \brief Utility functions to sanitize C++ snippets
 *
 * \date Wed Dec 19 23:17:53 MSK 2012 -- Initial design
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
#include <src/sanitize_snippet.h>

// Standard includes
#include <KDebug>
#include <cassert>
#include <map>
#include <vector>

namespace kate { namespace {
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
        QRegExp(R"--(std::(deque|list|vector)<(.*), std::allocator<\2\s?> >)--")
      , QString(R"--(std::\1<\2>)--")
    }
  , {
        QRegExp(R"--(std::(multimap|map)<(.*), (.*), std::less<\2\s?>, std::allocator<std::pair<const \2, \3\s?> > >)--")
      , QString(R"--(std::\1<\2, \3>)--")
    }
  , {
        QRegExp(R"--(std::(multiset|set)<(.*), std::less<\2\s?>, std::allocator<\2\s?> >)--")
      , QString(R"--(std::\1<\2>)--")
    }
  , {
        // Strip tail of boost::variant default template types
        QRegExp(R"--((, boost::detail::variant::void_)*>)--")
      , QString(">")
    }
  , {
        // Strip tail of boost::mpl containers default template types
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


QString sanitizeParams(QString&& params)
{
    return sanitizeTemplateCloseBrakets(
        sanitizeCompletionText(
            std::move(params)
          )
      );
}

QString sanitizePrefix(QString&& prefix)
{
    prefix = sanitizeCompletionText(std::move(prefix));
    prefix.replace(QLatin1String(" &"), QLatin1String("&"));
    prefix.replace(QLatin1String(" *"), QLatin1String("*"));
    return sanitizeTemplateCloseBrakets(std::move(prefix));
}

QString sanitizePlaceholder(QString&& str)
{
    /// \attention Qt 4.8 has only move assign but no move ctor for \c QString
    /// (as well as for other 'movable' types)... fucking idiots...
    QString result;
    result = std::move(str);
    auto last_space_pos = result.lastIndexOf(' ');
    if (last_space_pos != -1)
    {
        int count = 0;
        for (auto pos = ++last_space_pos, last = result.length(); pos < last; ++pos)
        {
            if (result[pos] == '*' || result[pos] == '&')
            {
                // ATTENTION Do not use `auto` for type of `ch`! Guess WHY...
                QChar ch = result[pos];
                result[pos] = result[pos - 1];
                result[pos - 1] = ch;
                ++last_space_pos;
                continue;
            }
            if (result[pos] != '_')
                break;
            ++count;
        }
        if (count)
            result.remove(last_space_pos, count);
    }
    return result;
}

std::pair<bool, QString> sanitize(
    QString text
  , const PluginConfiguration::sanitize_rules_list_type& sanitize_rules
  )
{
    kDebug(DEBUG_AREA) << "Sanitize snippet: " << text;
    bool result = true;
    QString output = std::move(text);
    // Try to sanitize completion items
    for (const auto& rule : sanitize_rules)
    {
        assert("A valid regex expected here!" && rule.first.isValid());
        kDebug(DEBUG_AREA) << "Trying " << rule.first.pattern() << " w/ replace text " << rule.second;
        if (rule.second.isEmpty())
        {
            // If replace text is empty, just ignore completion item if it matches...
            if (text.contains(rule.first))
            {
                result = false;                             // Must skip this item!
                output.clear();
                break;                                      // No need to match other rules!
            }
        }
        else
        {
            output.replace(rule.first, rule.second);
            kDebug(DEBUG_AREA) << "  output: " << output;
        }
    }
    return std::make_pair(result, output);
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
