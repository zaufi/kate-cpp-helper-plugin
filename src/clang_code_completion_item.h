/**
 * \file
 *
 * \brief Class \c kate::ClangCodeCompletionItem (interface)
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

#pragma once

// Project specific includes

// Standard includes
#include <clang-c/Index.h>
#include <KTextEditor/CodeCompletionModel>
#include <QtCore/QMap>
#include <QtCore/QPair>
#include <QtCore/QStringList>

namespace kate {

/**
 * \brief A class to represent all required info to display a completion item
 */
class ClangCodeCompletionItem
{
public:
    /// Type to hold data required to render a templated snipped to a user
    struct CompletionTemplateData
    {
        QString m_tpl;
        QMap<QString, QString> m_values;
        bool m_is_function;
    };

    /// Default constructor
    ClangCodeCompletionItem()
      : m_priority(0)
      , m_kind(CXCursor_UnexposedDecl)
    {}
    /// Initialize all fields
    ClangCodeCompletionItem(
        const QString& parent
      , const QString& before
      , const QString& text
      , const QString& after
      , const QStringList& placeholders
      , const int optional_placeholders_pos
      , const unsigned priority
      , const CXCursorKind kind
      , const bool is_deprecated = false
      )
      : m_parent(parent)
      , m_before(before.trimmed())                          // types w/ '&' may have a space: kick it!
      , m_text(text)
      , m_after(after)
      , m_placeholders(placeholders)
      , m_optional_placeholders_pos(optional_placeholders_pos)
      , m_priority(priority)
      , m_kind(kind)
      , m_deprecated(is_deprecated)
    {
    }

    QVariant data(const QModelIndex&, int, bool) const;
    KTextEditor::CodeCompletionModel::CompletionProperty completionProperty() const;
    QVariant icon() const;
    const QString& parentText() const                       ///< Return a parent (scope) text to display
    {
        return m_parent;
    }
    /// Get a string to be inserted and column position withing the string
    QPair<QString, int> executeCompletion() const;
    CompletionTemplateData getCompletionTemplate() const;

    /// Get cursor kind for this completion item
    CXCursorKind kind() const
    {
        return m_kind;
    }

private:

    QString renderPrefix() const;
    QString renderPlaceholders(const QString&) const;

    static const int NO_OPTIONAL_PLACEHOLDERS = -1;

    QString m_parent;                                       ///< Parent context of the current completion item
    QString m_before;                                       ///< Everything \e before typed text (return type)
    QString m_text;                                         ///< Text to paste
    QString m_after;                                        ///< Everything \e after typed text (arguments)
    QStringList m_placeholders;                             ///< Parameters to substitute
    int m_optional_placeholders_pos;                        ///< Position of the first optional placeholder
    unsigned m_priority;
    CXCursorKind m_kind;                                    ///< Cursor kind of this completion
    bool m_deprecated;                                      ///< Is current item deprecated?
};

}                                                           // namespace kate
// kate: hl C++11/Qt4;
