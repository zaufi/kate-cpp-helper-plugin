/**
 * \file
 *
 * \brief Class \c kate::ClangCodeCompletionModel (interface)
 *
 * \date Sun Nov 18 13:31:27 MSK 2012 -- Initial design
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
#include "clang_code_completion_item.h"

// Standard includes
#include <clang-c/Index.h>
#if __GNUC__
# pragma GCC push_options
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif                                                      // __GNUC__
#include <KDE/KTextEditor/CodeCompletionModel>
#include <KDE/KTextEditor/CodeCompletionModelControllerInterface>
#if __GNUC__
# pragma GCC pop_options
#endif                                                      // __GNUC__
#include <vector>

namespace kate {
// forward declarations
class CppHelperPlugin;
class DiagnosticMessagesModel;

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
    /**
     * Construct from parent \c QObject, plugin
     * pointer (to access configuration data) and
     * reference to diagnistic messages manager
     */
    ClangCodeCompletionModel(QObject*, CppHelperPlugin*, DiagnosticMessagesModel&);

    //BEGIN KTextEditor::CodeCompletionModel overrides
    virtual bool shouldStartCompletion(
        KTextEditor::View*
      , const QString&
      , bool
      , const KTextEditor::Cursor&
      ) override;
    /// Generate completions for given range
    virtual void completionInvoked(
        KTextEditor::View*
      , const KTextEditor::Range&
      , InvocationType
      ) override;
    virtual QModelIndex index(int, int, const QModelIndex&) const override;
    virtual int columnCount(const QModelIndex&) const override;
    virtual int rowCount(const QModelIndex&) const override;
    /// Make an index of a parent node
    virtual QModelIndex parent(const QModelIndex&) const override;
    /// Respond w/ data for particular completion entry
    virtual QVariant data(const QModelIndex&, int) const override;
    virtual void executeCompletionItem2(
        KTextEditor::Document*
      , const KTextEditor::Range&
      , const QModelIndex&
      ) const override;
    //END KTextEditor::CodeCompletionModel overrides

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

    QVariant getGroupData(const QModelIndex&, int) const;
    QVariant getItemData(const QModelIndex&, int) const;
    QVariant getItemHighlightData(const QModelIndex&, int) const;

    CppHelperPlugin* const m_plugin;
    DiagnosticMessagesModel& m_diagnostic_model;
    KTextEditor::View* m_current_view;
    groups_list_type m_groups;                              ///< Level one nodes
};

}                                                           // namespace kate
// kate: hl C++/Qt4;
