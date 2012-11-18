/**
 * \file
 *
 * \brief Class \c kate::ClangCodeCompletionModel (implementation)
 *
 * \date Sun Nov 18 13:31:27 MSK 2012 -- Initial design
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
#include <src/clang_code_completion_model.h>

// Standard includes

namespace kate {
ClangCodeCompletionModel::ClangCodeCompletionModel(
    QObject* parent
  , IncludeHelperPlugin* plugin
  )
  : KTextEditor::CodeCompletionModel2(parent)
  , m_plugin(plugin)
{
    
}

void ClangCodeCompletionModel::completionInvoked(
    KTextEditor::View* view
  , const KTextEditor::Range& range
  , InvocationType invocationType
  )
{
    if (invocationType == KTextEditor::CodeCompletionModel::UserInvocation)
    {
        kDebug() << "It seems user has invoked comletion at " << range;
    }
}

QVariant ClangCodeCompletionModel::data(const QModelIndex&, int) const
{
    return QVariant();
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
