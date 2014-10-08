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

    /// \name KTextEditor::CodeCompletionModelControllerInterface3
    //@{
    /// Check if line starts w/ \c #include and \c '"' or \c '<' just pressed
    virtual bool shouldStartCompletion(
        KTextEditor::View*
      , const QString&
      , bool
      , const KTextEditor::Cursor&
      ) override;

    /// Check if we've done w/ \c #include filename completion
    virtual bool shouldAbortCompletion(
        KTextEditor::View*
      , const KTextEditor::Range&
      , const QString&
      ) override;

    /// Returns the completion range that will be used for the current completion
    virtual KTextEditor::Range completionRange(
        KTextEditor::View*
      , const KTextEditor::Cursor&
      ) override;
    //@}

    /// \name KTextEditor::CodeCompletionModel overrides
    //@{
    virtual QModelIndex index(int, int, const QModelIndex&) const override;

    /// Get columns count (the only one)
    virtual int columnCount(const QModelIndex&) const override;

    /// Get rows count
    virtual int rowCount(const QModelIndex&) const override;

    /// Get parent's index
    virtual QModelIndex parent(const QModelIndex&) const override;

    /// Respond w/ data for particular completion entry
    virtual QVariant data(const QModelIndex&, int) const override;

    /// Insert a current completion item into a document
    virtual void executeCompletionItem2(
        KTextEditor::Document*
      , const KTextEditor::Range&
      , const QModelIndex&
      ) const override;

    /// Generate completions for given range
    virtual void completionInvoked(
        KTextEditor::View*
      , const KTextEditor::Range&
      , InvocationType
      ) override;
    //@}

private:
    enum Level                                              ///< Internal node ID constants
    {
        ITEM                                                ///< Leaf node (particular completion item)
      , GROUP                                               ///< Group node (w/ \e "Filesystem" text)
    };

    struct CompletionItem
    {
        QString text;                                       ///< Completion text
        bool is_directory = false;                          ///< \c true if item is a directory

        CompletionItem(QString, bool);
    };

    /// Update \c m_completions for given string
    void updateCompletionList(const QString&, const QString&);
    /// Scan for files and dirs
    void updateCompletionListPath(
        const QString&
      , const QStringList&
      , const QStringList&
      , const QStringList&
      );

    CppHelperPlugin* const m_plugin;
    std::vector<CompletionItem> m_completions;              ///< List of completion items
    QChar m_closer = 0;                                     ///< Character to be used to finalize \c #include
    bool m_should_complete = false;
};

}                                                           // namespace kate
// kate: hl C++/Qt4;
