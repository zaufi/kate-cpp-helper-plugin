/**
 * \file
 *
 * \brief Class \c kate::ClangCodeCompletionItem (implementation)
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

// Project specific includes
#include <src/clang_code_completion_item.h>

// Standard includes
#include <KTextEditor/CodeCompletionModel>
#include <cassert>

namespace kate {
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
        case Qt::DisplayRole:
            switch (index.column())
            {
                case KTextEditor::CodeCompletionModel::Name:
                    // Ok put placeholders into 
                    result = m_text;
                    break;
                case KTextEditor::CodeCompletionModel::Prefix:
                    result = m_before;
                    break;
                case KTextEditor::CodeCompletionModel::Postfix:
                    break;
                case KTextEditor::CodeCompletionModel::Arguments:
                    result = m_after;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return result;
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
