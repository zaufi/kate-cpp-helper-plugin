/**
 * \file
 *
 * \brief Class \c kate::ClangCodeCompletionItem (interface)
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

#ifndef __SRC__CLANG_CODE_COMPLETION_ITEM_H__
# define __SRC__CLANG_CODE_COMPLETION_ITEM_H__

// Project specific includes

// Standard includes
# include <clang-c/Index.h>
# include <KTextEditor/CodeCompletionModel>
# include <QtCore/QStringList>

namespace kate {

/**
 * \brief [Type brief class description here]
 *
 * [More detailed description here]
 *
 */
class ClangCodeCompletionItem
{
public:
    /// Default constructor
    ClangCodeCompletionItem()
      : m_priority(0)
      , m_kind(CXCursor_UnexposedDecl)
    {}
    /// Initialize all fields
    ClangCodeCompletionItem(
        const QString& before
      , const QString& text
      , const QString& after
      , const QStringList& placeholders
      , const unsigned priority
      , const CXCursorKind kind
      )
      : m_before(before)
      , m_text(text)
      , m_after(after)
      , m_placeholders(placeholders)
      , m_priority(priority)
      , m_kind(kind)
    {
    }

    QVariant data(const QModelIndex&, const int) const;
    KTextEditor::CodeCompletionModel::CompletionProperty completionProperty() const;

private:
    QString renderPlaceholders(const QString&) const;

    QString m_before;                                       ///< Everything \e before typed text (return type)
    QString m_text;                                         ///< Text to paste
    QString m_after;                                        ///< Everything \e after typed text (arguments)
    QStringList m_placeholders;                             ///< Parameters to substitute
    unsigned m_priority;
    CXCursorKind m_kind;
};

}                                                           // namespace kate
#endif                                                      // __SRC__CLANG_CODE_COMPLETION_ITEM_H__
// kate: hl C++11/Qt4;
