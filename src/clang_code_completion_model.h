/**
 * \file
 *
 * \brief Class \c kate::ClangCodeCompletionModel (interface)
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

#ifndef __SRC__CLANG_CODE_COMPLETION_MODEL_H__
# define __SRC__CLANG_CODE_COMPLETION_MODEL_H__

// Project specific includes
# include <src/clang_code_completion_item.h>
# include <src/translation_unit.h>

// Standard includes
# include <clang-c/Index.h>
# if (__GNUC__ >=4 && __GNUC_MINOR__ >= 5)
#   pragma GCC push_options
#   pragma GCC diagnostic ignored "-Wdeprecated-declarations"
# endif                                                     // (__GNUC__ >=4 && __GNUC_MINOR__ >= 5)
# include <KTextEditor/CodeCompletionModel>
# include <KTextEditor/CodeCompletionModelControllerInterface>
# if (__GNUC__ >=4 && __GNUC_MINOR__ >= 5)
#   pragma GCC pop_options
# endif                                                     // (__GNUC__ >=4 && __GNUC_MINOR__ >= 5)
# include <KTextEdit>
# include <memory>
# include <vector>

namespace kate {
class IncludeHelperPlugin;                                  // forward declaration

/**
 * \brief [Type brief class description here]
 *
 * [More detailed description here]
 *
 */
class ClangCodeCompletionModel
  : public KTextEditor::CodeCompletionModel2
  , public KTextEditor::CodeCompletionModelControllerInterface3
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface3)

public:
    /// Default constructor
    ClangCodeCompletionModel(QObject*, IncludeHelperPlugin*, KTextEdit*);

    //BEGIN KTextEditor::CodeCompletionModel overrides
    /// Generate completions for given range
    void completionInvoked(KTextEditor::View*, const KTextEditor::Range&, InvocationType);
    /// Respond w/ data for particular completion entry
    QVariant data(const QModelIndex&, int) const;
    int columnCount(const QModelIndex&) const;
    int rowCount(const QModelIndex& parent) const;
    /// Make an index of a parent node
    QModelIndex parent(const QModelIndex& index) const;
    QModelIndex index(int row, int column, const QModelIndex& parent) const;
    bool shouldStartCompletion(KTextEditor::View*, const QString&, bool, const KTextEditor::Cursor&);
    void executeCompletionItem2(KTextEditor::Document*, const KTextEditor::Range&, const QModelIndex&) const;
    //END KTextEditor::CodeCompletionModel overrides

private Q_SLOTS:
    void invalidateTranslationUnit();

private:
    struct GroupInfo
    {
        unsigned m_priority;
        std::vector<ClangCodeCompletionItem> m_completions;

        GroupInfo() : m_priority(0) {}                      ///< Default ctor
        GroupInfo(GroupInfo&&) = default;                   ///< Default move ctor
        GroupInfo& operator=(GroupInfo&&) = default;        ///< Default move-assign operator
    };
    typedef std::vector<std::pair<QString, GroupInfo>> groups_list_type;

    /// Possible levels in a completion hierarchy
    /// \note Leaf nodes will contain an index to a group
    /// (and yes, it is supposed that gorups count is less than \c 0xcafe)
    enum Level
    {
        GROUP = 0xcafe                                      ///< Group node (level 1)
    };

    /// Helper function to collect unsaved files from current editor
    TranslationUnit::unsaved_files_list_type makeUnsavedFilesList(KTextEditor::Document*);

    IncludeHelperPlugin* m_plugin;
    KTextEdit* m_diagnostic_text;
    KTextEditor::View* m_current_view;
    std::unique_ptr<TranslationUnit> m_unit;
    groups_list_type m_groups;                              ///< Level one nodes
};


}                                                           // namespace kate
#endif                                                      // __SRC__CLANG_CODE_COMPLETION_MODEL_H__
// kate: hl C++11/Qt4;
