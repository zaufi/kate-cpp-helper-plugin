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
#include <QtCore/QRegExp>
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

}                                                           // namespace kate
// kate: hl C++11/Qt4;
