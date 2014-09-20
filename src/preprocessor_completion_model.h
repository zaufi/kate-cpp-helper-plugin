/**
 * \file
 *
 * \brief Class \c kate::PreprocessorCompletionModel (interface)
 *
 * \date Mon Sep  8 08:10:22 MSK 2014 -- Initial design
 */
/*
 * Copyright (C) 2011-2014 Alex Turbov, all rights reserved.
 * This is free software. It is licensed for use, modification and
 * redistribution under the terms of the GNU General Public License,
 * version 3 or later <http://gnu.org/licenses/gpl.html>
 *
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
class PreprocessorCompletionModel
  : public KTextEditor::CodeCompletionModel2
  , public KTextEditor::CodeCompletionModelControllerInterface3
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface3)

public:
    PreprocessorCompletionModel(QObject*, CppHelperPlugin*, DiagnosticMessagesModel&);

    /// \name KTextEditor::CodeCompletionModelControllerInterface3
    //@{
    virtual bool shouldStartCompletion(
        KTextEditor::View*
      , const QString&
      , bool
      , const KTextEditor::Cursor&
      ) override;
    /// Check if entered text is enough to guess what a user wants
    virtual bool shouldAbortCompletion(
        KTextEditor::View*
      , const KTextEditor::Range&
      , const QString&
      ) override;
#if 0
    /// Check if selected item can be executed after pressing a given character
    /// \todo Make it really work in kate!
    virtual bool shouldExecute(const QModelIndex&, QChar) override;
#endif
    //@}

    /// \name KTextEditor::CodeCompletionModel overrides
    //@{
    /// Generate completions for given range
    virtual void completionInvoked(
        KTextEditor::View*
      , const KTextEditor::Range&
      , InvocationType
      ) override;
    /// Build a model index instance for given row and column
    virtual QModelIndex index(int, int, const QModelIndex&) const override;
    /// Get columns count for a given parent index
    virtual int columnCount(const QModelIndex&) const override;
    /// Get rows count for a given parent index
    virtual int rowCount(const QModelIndex&) const override;
    /// Make an index of a parent node for a given one
    virtual QModelIndex parent(const QModelIndex&) const override;
    /// Respond w/ data for particular completion entry
    virtual QVariant data(const QModelIndex&, int) const override;
    virtual void executeCompletionItem2(
        KTextEditor::Document*
      , const KTextEditor::Range&
      , const QModelIndex&
      ) const override;
    //@}

private:
    enum Level                                              ///< Internal node ID constants
    {
        ITEM                                                ///< Leaf node (particular completion item)
      , GROUP                                               ///< Group node (w/ \e "Preprocessor" text)
    };

    struct CompletionItem
    {
        /**
         * Completion item text possible  with a special
         * character \c '|' indicating desired cursor position
         * after completion item gets selected.
         */
        QString text;
        /**
         * After this position it will be clear that
         * selected variant is the only remains, so no
         * need to wait while user will type the wole
         * word! Let's do this for him!
         */
        std::size_t complete_after;
    };

    const std::vector<CompletionItem> COMPLETIONS = {
        { "include <|>", 2 }
      , { "include \"|\"", 2 }
      , { "define |", 1 }
      , { "undef |", 1 }
      , { "ifdef |", 3 }
      , { "ifndef |", 3 }
      , { "else", 3 }
      , { "elif |", 3 }
      , { "endif", 2 }
      , { "pragma", 1 }
      };

    CppHelperPlugin* const m_plugin;
    DiagnosticMessagesModel& m_diagnostic_model;
    bool m_should_complete = false;
};

}                                                           // namespace kate
// kate: hl C++/Qt4;
