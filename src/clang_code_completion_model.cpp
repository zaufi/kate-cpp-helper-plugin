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
#include <src/clang_utils.h>
#include <src/translation_unit.h>
#include <src/include_helper_plugin.h>

// Standard includes
#include <KLocalizedString>                                 /// \todo Where is \c i18n() defiend?
#include <KTextEditor/Document>
#include <KTextEditor/View>

namespace kate {

ClangCodeCompletionModel::ClangCodeCompletionModel(
    QObject* parent
  , IncludeHelperPlugin* plugin
  )
  : KTextEditor::CodeCompletionModel2(parent)
  , m_plugin(plugin)
{
}

bool ClangCodeCompletionModel::shouldStartCompletion(
    KTextEditor::View* /*view*/
  , const QString& /*inserted_text*/
  , bool /*user_insertion*/
  , const KTextEditor::Cursor& /*position*/
  )
{
    return false;
}

void ClangCodeCompletionModel::completionInvoked(
    KTextEditor::View* view
  , const KTextEditor::Range& range
  , InvocationType invocationType
  )
{
    kDebug() << "some kind of comletion requested";
    // Do nothing if completion not called by user
    if (invocationType != KTextEditor::CodeCompletionModel::UserInvocation)
        return;

    KTextEditor::Document* doc = view->document();
    KUrl url = doc->url();
    if (!url.isValid() || url.isEmpty())
    {
        /// \todo Turn into a popup
        kWarning() << "U have to have a document on a disk before use code completion";
        return;
    }

    kDebug() << "It seems user has invoked comletion at " << range;

    // Remove everything collected before
    m_completions.clear();

    // Form command line parameters
    //  1) collect configured system and session dirs and make -I option series
    QStringList options = m_plugin->config().formCompilerOptions();
    //  2) append PCH options
    if (!m_plugin->config().pchFile().isEmpty())
        options << /*"-Xclang" << */"-include-pch" << m_plugin->config().pchFile().toLocalFile();
    // Form unsaved files list
    QVector<QPair<QString, QString>> unsaved_files;
    //  1) append this document to the list of unsaved files
    QString this_filename = url.toLocalFile();
    unsaved_files.push_back(qMakePair(this_filename, doc->text()));
    /// \todo Collect other unsaved files

    try
    {
        // Parse it!
        TranslationUnit unit(
            m_plugin->index()
          , url
          , options
          , TranslationUnit::ParseOptions::CompletionOptions
          , unsaved_files
          );
        // Try to make a comletion
        m_completions = unit.completeAt(
            unsigned(range.start().line() + 1)              // NOTE Kate count lines starting from 0
          , unsigned(range.start().column() + 1)            // NOTE Kate count columns starting from 0
        );
    }
    catch (const TranslationUnit::Exception& e)
    {
        kError() << "Fail to make a code completion: " << e.what();
    }
}

QVariant ClangCodeCompletionModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    // grouping of completions
    if (!index.parent().isValid())
    {
        if (role == Qt::DisplayRole)
            return i18n("Code Completions");
        if (role == KTextEditor::CodeCompletionModel::GroupRole)
            return Qt::DisplayRole;
        return QVariant();
    }
    // completions
    if (!index.isValid() || index.row() < 0 || index.row() >= m_completions.size())
        return QVariant();

#if 0
    if (role == CompletionRole)
        kDebug() << "---------------------------- CompletionRole requested for " << index;
#endif

    return m_completions[index.row()].data(index, role);
}

int ClangCodeCompletionModel::columnCount(const QModelIndex&) const
{
    return 1;
}

int ClangCodeCompletionModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid() && !m_completions.empty())
        return 1;                                           // one toplevel node (group header)
    if(parent.parent().isValid())
        return 0;                                           // we don't have sub children

    return m_completions.size();                            // only the children
}

QModelIndex ClangCodeCompletionModel::parent(const QModelIndex& index) const
{
    // make a ref to root node from level 1 nodes,
    // otherwise return an invalid node.
    return index.internalId() ? createIndex(0, 0, 0) : QModelIndex();
}

QModelIndex ClangCodeCompletionModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!parent.isValid())
    {
        if (row == 0)
            return createIndex(row, column, 0);             // header  index
        return QModelIndex();
    }                                                       // we only have header and children, no subheaders
    if (parent.parent().isValid())
        return QModelIndex();

    if (row < 0 || row >= m_completions.size() || column < 0 || column >= ColumnCount )
        return QModelIndex();

    return createIndex(row, column, 1);                     // normal item index
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
