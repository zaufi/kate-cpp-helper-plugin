/**
 * \file
 *
 * \brief Class \c kate::utils (implementation)
 *
 * \date Mon Feb  6 04:12:51 MSK 2012 -- Initial design
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
#include <src/utils.h>

// Standard includes
#include <KDebug>
#include <cassert>

namespace kate {
/**
 * Return a range w/ filename if given line contains a valid \c #include
 * directive, empty range otherwise.
 *
 * \param line a string w/ line to parse
 * \param strict return failure if closing \c '>' or \c '"' missed
 *
 * \return a range w/ filename of \c #include directive
 */
KTextEditor::Range parseIncludeDirective(const QString& line, const bool strict)
{
    kDebug() << "text2parse=" << line << ", strict=" << strict;
    KTextEditor::Range result;
    enum State {
        skipInitialSpaces
      , foundHash
      , checkInclude
      , skipSpaces
      , foundOpenChar
      , findCloseChar
      , stop
    };
    int start = 0;
    int end = 0;
    int tmp = 0;
    QChar close = 0;
    State state = skipInitialSpaces;
    for (int pos = 0; pos < line.length() && state != stop; ++pos)
    {
        switch (state)
        {
            case skipInitialSpaces:
                if (!line[pos].isSpace())
                {
                    if (line[pos] == '#')
                    {
                        state = foundHash;
                        continue;
                    }
                    return result;
                }
                break;
            case foundHash:
                if (line[pos].isSpace())
                    continue;
                else
                    state = checkInclude;
                // NOTE No `break' here!
            case checkInclude:
                if ("include"[tmp++] != line[pos])
                    return result;
                if (tmp == 7)
                    state = skipSpaces;
                break;
            case skipSpaces:
                if (line[pos].isSpace())
                    continue;
                close = (line[pos] == '<') ? '>' : (line[pos] == '"') ? '"' : 0;
                if (close == 0)
                    return result;
                state = foundOpenChar;
                break;
            case foundOpenChar:
                state = findCloseChar;
                start = pos;
                end = pos;
                // NOTE No `break' here!
            case findCloseChar:
                if (line[pos] == close)
                {
                    state = stop;
                    end = pos;
                }
                else if (line[pos].isSpace())
                {
                    if (strict)
                        return result;
                    state = stop;
                    end = pos;
                }
                break;
            case stop:
            default:
                assert(!"Parsing FSM broken!");
        }
    }
    // Check state after EOL occurs
    switch (state)
    {
        case skipInitialSpaces:
        case foundHash:
        case checkInclude:
        case skipSpaces:
            return result;
        case foundOpenChar:
            return KTextEditor::Range(0, line.length(), 0, line.length() + 1);
        case findCloseChar:
            return KTextEditor::Range(0, start, 0, line.length());
        case stop:
            break;
        default:
            assert(!"Parsing FSM broken!");
    }
    return KTextEditor::Range(0, start, 0, end);
}

}                                                           // namespace kate
