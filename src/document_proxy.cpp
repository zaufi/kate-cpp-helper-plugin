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

KTextEditor::Range DocumentProxy::getWordUnderCursor(const KTextEditor::Cursor& pos)
{
    auto is_space = [](const QChar c) { return !c.isLetterOrNumber(); };

    // Get range of nonspace characters starting from current cursor position
    auto line = pos.line();
    KTextEditor::Range range_after_cursor = {
        line
      , pos.column()
      , line
      , m_doc_ptr->lineLength(line)
      };
    kDebug() << "range_after_cursor =" << range_after_cursor;
    KTextEditor::Cursor after = scanChars(range_after_cursor, is_space);

    // Get range of nonspace characters before cursor position
    KTextEditor::Range range_before_cursor = {line, 0, line, pos.column()};
    kDebug() << "range_before_cursor =" << range_before_cursor;
    KTextEditor::Cursor before = scanCharsReverse(range_before_cursor, is_space);

    // Merge both result ranges
    if (before.isValid() && after.isValid())
        return {line, before.column() + 1, line, after.column()};
    else if (before.isValid())
        return {line, before.column() + 1, line, m_doc_ptr->lineLength(line)};
    else if (after.isValid())
        return {line, 0, line, after.column()};
    return {KTextEditor::Cursor::invalid(), KTextEditor::Cursor::invalid()};
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
