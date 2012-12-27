/**
 * \file
 *
 * \brief Class \c kate::DocumentProxy (implementation)
 *
 * \date Tue Dec 25 08:45:27 MSK 2012 -- Initial design
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
#include <src/document_proxy.h>

// Standard includes

namespace kate {

KTextEditor::Range DocumentProxy::getIdentifierUnderCursor(const KTextEditor::Cursor& pos)
{
    auto is_space = [](const QChar c) { return !c.isLetterOrNumber(); };

    // Get range of alphanumeric characters starting from current cursor position
    auto line = pos.line();
    KTextEditor::Range range_after_cursor = {
        line
      , pos.column()
      , line
      , m_doc_ptr->lineLength(line)
      };
#if 0
    kDebug() << "range_after_cursor =" << range_after_cursor;
#endif
    KTextEditor::Cursor after = scanChars(range_after_cursor, is_space);

    // Get range of nonspace characters before cursor position
    KTextEditor::Range range_before_cursor = {line, 0, line, pos.column()};
#if 0
    kDebug() << "range_before_cursor =" << range_before_cursor;
#endif
    KTextEditor::Cursor before = scanCharsReverse(range_before_cursor, is_space);

    // Merge both result ranges
    if (before.isValid() && after.isValid())
        return {line, before.column() + 1, line, after.column()};
    else if (before.isValid())
        return {line, before.column() + 1, line, m_doc_ptr->lineLength(line)};
    else if (after.isValid())
        return {line, 0, line, after.column()};
    return KTextEditor::Range::invalid();
}

/**
 * \todo Move a lambda to a functior, so it can be reused by \c wordAfterCursor.
 * But there is some problem: functiors for \c scanChars and \c scanCharsReverse
 * can't have a state :-( ... at leas nowadays.
 */
KTextEditor::Range DocumentProxy::firstWordBeforeCursor(const KTextEditor::Cursor& pos)
{
    // Make a range statring from cursor position to the end of line
    const auto line = pos.line();
    const auto column = pos.column();
    const KTextEditor::Range range2scan = {line, 0, line, column};
    bool skip_spaces = true;
    int spaces_count = 0;
    KTextEditor::Cursor word_start = scanCharsReverse(
        range2scan
      , [&](const QChar c)
        {
            if (skip_spaces)                                // do we still need to skip spaces?
            {
                skip_spaces = c.isSpace();                  // turn OFF space skipper on first nonspace char
                spaces_count += int(skip_spaces);           // increment leading spaces count
                return false;
            }
            return c.isSpace();                             // stop on next space char
        }
      );
    // did we found smth?
    if (word_start.isValid())
        return {line, word_start.column() + 1, line, column - spaces_count - 1};
    // check is there anything 'cept spaces before cursror:
    // count of skipped spaces expected to be less than a given cursor position (minus one)
    if (skip_spaces < column - 1)
        return {line, 0, line, column - skip_spaces - 1};
    // if nothing but spaces, return an invalid range
    return KTextEditor::Range::invalid();
}

KTextEditor::Range DocumentProxy::firstWordAfterCursor(const KTextEditor::Cursor& pos)
{
    // Make a range statring from cursor position to the end of line
    const auto line = pos.line();
    const auto column = pos.column();
    const auto line_length = m_doc_ptr->lineLength(line);
    const KTextEditor::Range range2scan = {line, column, line, line_length};
    bool skip_spaces = true;
    int spaces_count = 0; 
    KTextEditor::Cursor word_start = scanChars(
        range2scan
      , [&](const QChar c)
        {
            if (skip_spaces)                                // do we still need to skip spaces?
            {
                skip_spaces = c.isSpace();                  // turn OFF space skipper on first nonspace char
                spaces_count += int(skip_spaces);           // increment leading spaces count
                return false;
            }
            return c.isSpace();                             // stop on next space char
        }
      );
#if 0
    kDebug() << "skip_spaces=" << skip_spaces << ", spaces_count="<< spaces_count << ", c="<< word_start;
#endif
    // did we found smth?
    if (word_start.isValid())
        return {line, column + spaces_count, line, word_start.column()};
    // check is there anything 'cept spaces on a line:
    // current pos + skipped spaces should be less than length of the line
#if 0
    kDebug() << "line_length=" << line_length<< ", column="<< column;
#endif
    if ((column + skip_spaces) < line_length)
        return {line, column + spaces_count, line, line_length};
    // if nothing but spaces, return an invalid range
    return KTextEditor::Range::invalid();
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
