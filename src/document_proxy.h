/**
 * \file
 *
 * \brief Class \c kate::DocumentProxy (interface)
 *
 * \date Tue Dec 25 08:45:27 MSK 2012 -- Initial design
 *
 * \todo Unit tests are badly required. Dunno is it possible at all being
 * not in the \e kate.git
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

#pragma once

// Project specific includes

// Standard includes
#include <KDE/KTextEditor/Cursor>
#include <KDE/KTextEditor/Document>
#include <KDE/KTextEditor/Range>
#include <algorithm>
#include <cassert>
#include <limits>

namespace kate {

/**
 * \brief Class to decorate \e standard \c KTextEditor::Document class
 *
 * Most of the time plugin code has a pointer to document (instance of \c KTextEditor::Document).
 * Latter class has just minimum of required methods. So this class extends a document
 * w/ some cool functionality.
 *
 * \note There is no overload dereference operator cuz most of the time Kate API accept
 * documents by raw pointers, so there is no need to dereference it in user (plugin) code.
 *
 * \todo Add a function to get highligting mode at specified location
 */
class DocumentProxy
{
public:
    /// Build a proxy from a raw pointer
    DocumentProxy(KTextEditor::Document* ptr = nullptr)
      : m_doc_ptr(ptr)
    {}
    /// Default copy ctor
    DocumentProxy(const DocumentProxy&) = default;
    /// Default copy-assign operator
    DocumentProxy& operator=(const DocumentProxy&) = default;
    /// Default move ctor
    DocumentProxy(DocumentProxy&&) = default;
    /// Default move-assign operator
    DocumentProxy& operator=(DocumentProxy&&) = default;

    /// Allow implicit conversion to original pointer
    operator KTextEditor::Document*()
    {
        return m_doc_ptr;
    }
    operator const KTextEditor::Document*() const
    {
        return m_doc_ptr;
    }

    /// Override member access
    KTextEditor::Document* operator->()
    {
        return m_doc_ptr;
    }
    const KTextEditor::Document* operator->() const
    {
        return m_doc_ptr;
    }

    /// Generic function to scan a document forward while predicate is not true
    template <typename Predicate>
    KTextEditor::Cursor scanChars(
        const KTextEditor::Range&
      , Predicate
      );
    /// Generic function to scan a range from end to start while predicate is not true
    template <typename Predicate>
    KTextEditor::Cursor scanCharsReverse(
        const KTextEditor::Range&
      , Predicate
      );

    KTextEditor::Range getIdentifierUnderCursor(const KTextEditor::Cursor&);
    KTextEditor::Range firstWordBeforeCursor(const KTextEditor::Cursor&);
    KTextEditor::Range firstWordAfterCursor(const KTextEditor::Cursor&);

private:
    template <typename Predicate>
    KTextEditor::Cursor handleLine(const int, int, const int, Predicate);
    template <typename Predicate>
    KTextEditor::Cursor handleLineReverse(const int, const int, int, Predicate);

    KTextEditor::Document* m_doc_ptr;
};

template <typename Predicate>
KTextEditor::Cursor DocumentProxy::handleLine(
    const int line
  , int column
  , const int last_column
  , Predicate p
  )
{
    assert("Sanity check!" && column < last_column);
    assert("Sanity check!" && line < m_doc_ptr->lines());

    const auto& text = m_doc_ptr->line(line);
    assert("Sanity check!" && last_column <= text.length());

#if 0
    kDebug(DEBUG_AREA) << "line =" << line << ", col =" << column << ", last_col =" << last_column;
#endif

    for (; column < last_column; ++column)
        if (p(text[column]))
            return {line, column};

    return KTextEditor::Cursor::invalid();
}

template <typename Predicate>
inline KTextEditor::Cursor DocumentProxy::scanChars(
    const KTextEditor::Range& range
  , Predicate p
  )
{
    assert("A valid range expected" && range.isValid());
    assert("A valid range within document expected" && m_doc_ptr->documentRange().contains(range));
    /// \todo Assert that \c Predicate is callable w/ expected signature

    if (range.isEmpty())
        return KTextEditor::Cursor::invalid();

    if (range.onSingleLine())
        return handleLine(range.start().line(), range.start().column(), range.end().column(), p);

    auto result = KTextEditor::Cursor::invalid();

    int mb_initial_line_off = int(range.start().column() != 0);
    // 0) Handle the first line of the range, if start column somewhere in the middle
    if (mb_initial_line_off)
        result = handleLine(
            range.start().line()
          , range.start().column()
          , m_doc_ptr->lineLength(range.start().line())
          , p
          );

#if 0
    kDebug(DEBUG_AREA) << "#BlockStart:" << result;
#endif

    // 1) Handle the middle of the block (i.e. only full lines from column 0 to the end of line)
    if (!result.isValid())
    {
        for (
            int line = range.start().line() + mb_initial_line_off
          , last_line = range.end().line()
          ; line < last_line && !result.isValid()
          ; ++line
          ) result = handleLine(line, 0, m_doc_ptr->lineLength(line), p)
          ;
    }

#if 0
    kDebug(DEBUG_AREA) << "#BlockMiddle:" << result;
#endif

    // 2) Handle the tail of the range block
    if (!result.isValid())
        result = handleLine(
            range.end().line()
          , 0
          , std::min(range.end().column(), m_doc_ptr->lineLength(range.end().line()))
          , p
          );

#if 0
    kDebug(DEBUG_AREA) << "#BlockEnd:" << result;
#endif

    return result;
}

template <typename Predicate>
KTextEditor::Cursor DocumentProxy::handleLineReverse(
    const int line
  , const int first_column
  , int column
  , Predicate p
  )
{
    assert("Sanity check!" && line < m_doc_ptr->lines());
    assert("Sanity check!" && first_column < column);

    const auto& text = m_doc_ptr->line(line);
    assert("Sanity check!" && column <= text.length());

#if 0
    kDebug(DEBUG_AREA) << "line =" << line << ", first_col =" << first_column << ", col =" << column;
    kDebug(DEBUG_AREA) << "size =" << text.size() << ", text =" << text;
#endif

    // Iterate over text starting from a column which is previous for a given
    for (--column; first_column < column; --column)
        if (p(text[column]))
            return {line, column};

    return KTextEditor::Cursor::invalid();
}


template <typename Predicate>
inline KTextEditor::Cursor DocumentProxy::scanCharsReverse(
    const KTextEditor::Range& r
  , Predicate p
  )
{
    assert("A valid range expected" && r.isValid());
    assert("A valid range within document expected" && m_doc_ptr->documentRange().contains(r));
    /// \todo Assert that \c Predicate is callable w/ expected signature

    if (r.isEmpty())
        return KTextEditor::Cursor::invalid();

    if (r.onSingleLine())
        return handleLineReverse(r.start().line(), r.start().column(), r.end().column(), p);

    auto result = KTextEditor::Cursor::invalid();

    auto range = r;
    int mb_initial_line_off = int(range.end().column() != m_doc_ptr->lineLength(range.end().line()));
    // Skip one 'end' position
    if (range.end().column())                               // Is there anything before an end position?
        range.end().setColumn(range.end().column() - 1);    // Yep, just move into it!
    else                                                    // No, lets move to the end of a previous line
    {
        range.end().setLine(range.end().line() - 1);        // Fix last line
        // Fix last column
        range.end().setColumn(m_doc_ptr->lineLength(range.end().line()));
        // Is range now on a single line?
        if (range.onSingleLine())
            return handleLineReverse(range.start().line(), range.start().column(), range.end().column(), p);
        // No, just remember that 'main' block would start from last line...
        mb_initial_line_off = 0;
    }

#if 0
    kDebug(DEBUG_AREA) << "#Range2Process:" << range;
#endif

    // 0) Handle block end
    if (mb_initial_line_off)
        result = handleLineReverse(range.end().line(), 0, range.end().column(), p);

#if 0
    kDebug(DEBUG_AREA) << "#BlockEnd:" << result;
#endif

    // 1) Handle the middle of the block (i.e. only full lines from column 0 to the end of line)
    if (!result.isValid())
    {
        for (
            int line = range.end().line() - mb_initial_line_off
          , last_line = range.start().line()
          ; line < last_line && !result.isValid()
          ; --line
          ) result = handleLineReverse(line, 0, m_doc_ptr->lineLength(line), p)
          ;
    }

#if 0
    kDebug(DEBUG_AREA) << "#BlockMiddle:" << result;
#endif

    if (!result.isValid())
    {
        result = handleLineReverse(
            range.start().line()
          , range.start().column()
          , m_doc_ptr->lineLength(range.start().line())
          , p
          );
    }

#if 0
    kDebug(DEBUG_AREA) << "#BlockStart:" << result;
#endif

    return result;
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
