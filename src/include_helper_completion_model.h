/**
 * \file
 *
 * \brief Class \c kate::IncludeHelperCompletionModel (interface)
 *
 * \date Mon Feb  6 06:12:41 MSK 2012 -- Initial design
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
#if (__GNUC__ >=4 && __GNUC_MINOR__ >= 5)
# pragma GCC push_options
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif                                                      // (__GNUC__ >=4 && __GNUC_MINOR__ >= 5)
#include <KTextEditor/CodeCompletionModel>
#include <KTextEditor/CodeCompletionModelControllerInterface>
#if (__GNUC__ >=4 && __GNUC_MINOR__ >= 5)
# pragma GCC pop_options
#endif                                                      // (__GNUC__ >=4 && __GNUC_MINOR__ >= 5)

namespace kate {
class CppHelperPlugin;                                      // forward declaration

/**
 * \brief [Type brief class description here]
 *
 * [More detailed description here]
 *
 */
class IncludeHelperCompletionModel
  : public KTextEditor::CodeCompletionModel2
  , public KTextEditor::CodeCompletionModelControllerInterface3
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface3)

public:
    IncludeHelperCompletionModel(QObject*, CppHelperPlugin*);

    //BEGIN KTextEditor::CodeCompletionModel overrides

    /// Generate completions for given range
    void completionInvoked(KTextEditor::View*, const KTextEditor::Range&, InvocationType);
    void executeCompletionItem2(KTextEditor::Document*, const KTextEditor::Range&, const QModelIndex&) const;
    KTextEditor::Range completionRange(KTextEditor::View*, const KTextEditor::Cursor&);
    /// Respond w/ data for particular completion entry
    QVariant data(const QModelIndex&, int) const;
    /// Check if line starts w/ \c #include and \c '"' or \c '<' just pressed
    bool shouldStartCompletion(KTextEditor::View*, const QString&, bool, const KTextEditor::Cursor&);
    /// Check if we've done w/ \c #include filename completion
    bool shouldAbortCompletion(KTextEditor::View*, const KTextEditor::Range&, const QString&);
    QModelIndex index(int, int, const QModelIndex&) const;
    int rowCount(const QModelIndex& parent) const
    {
        if (parent.parent().isValid())
            return 0;
        return parent.isValid()
          ? m_dir_completions.size() + m_file_completions.size()
          : 1                                               // No parent -- root node...
          ;
    }
    int columnCount(const QModelIndex&) const
    {
        return 1;
    }
    QModelIndex parent(const QModelIndex& index) const
    {
        // make a ref to root node from level 1 nodes,
        // otherwise return an invalid node.
        return index.internalId() ? createIndex(0, 0, 0) : QModelIndex();
    }
    //END KTextEditor::CodeCompletionModel overrides

private:
    /// Update \c m_completions for given string
    void updateCompletionList(const QString&, const bool);

    CppHelperPlugin* m_plugin;
    QStringList m_dir_completions;                          ///< List of dirs suggested
    QStringList m_file_completions;                         ///< List of files suggested
    QChar m_closer;
    bool m_should_complete;
};

}                                                           // namespace kate
// kate: hl C++11/Qt4;
