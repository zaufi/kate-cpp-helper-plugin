/**
 * \file
 *
 * \brief Class \c kate::PreprocessorCompletionModel (implementation)
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

// Project specific includes
#include "preprocessor_completion_model.h"
#include "utils.h"

// Standard includes
#include <KDE/KTextEditor/Document>
#include <KDE/KTextEditor/HighlightInterface>
#include <KDE/KTextEditor/View>
#include <KDE/KLocalizedString>
#include <algorithm>
#include <cassert>

namespace kate {

/**
 * Construct from parent \c QObject, plugin
 * pointer (to access configuration data) and
 * reference to diagnistic messages manager
 */
PreprocessorCompletionModel::PreprocessorCompletionModel(
    QObject* const parent
  , CppHelperPlugin* const plugin
  , DiagnosticMessagesModel& dm
  )
  : KTextEditor::CodeCompletionModel2(parent)
  , m_plugin{plugin}
  , m_diagnostic_model{dm}
{
}

/**
 * We don't care about how many lines possible was inserted. Just consider
 * a current one.
 */
bool PreprocessorCompletionModel::shouldStartCompletion(
    KTextEditor::View* const view
  , const QString& /*inserted_text*/
  , const bool /*user_insertion*/
  , const KTextEditor::Cursor& position
  )
{
    m_should_complete = false;
    auto* const doc = view->document();                     // get current document
    auto* const iface = qobject_cast<KTextEditor::HighlightInterface*>(doc);
    // Do nothing if no highlighting interface or not suitable document or
    // a place within it... (we won't to complete smth in non C++ files or comments for example)
    if (!iface || !isSuitableDocumentAndHighlighting(doc->mimeType(), iface->highlightingModeAt(position)))
        return false;

    auto text_before = doc->text({KTextEditor::Cursor(position.line(), 0), position});
    kDebug(DEBUG_AREA) << "text_before=" << text_before;
    /// Check if current line starts w/ \c '#' which is a sign of a preprocessor directive.
    if (text_before[0] == '#')
    {
        text_before = text_before.remove(0, 1).trimmed();
        kDebug(DEBUG_AREA) << "again text_before=" << text_before;
        /// Then make sure the text after it, is a subset of some
        /// hardcoded item from the \c COMPLETIONS table.
        m_should_complete = text_before.isEmpty() || std::any_of(
            begin(COMPLETIONS)
          , end(COMPLETIONS)
          , [&text_before](const auto& item)
            {
                auto text = item.text;
                const auto end_of_first_word = text.indexOf(' ');
                if (end_of_first_word != -1)
                    // Strip tail of the completion item... only first word is interesting!
                    text = text.left(end_of_first_word);
                return text_before.size() < text.size() && text.startsWith(text_before);
            }
          );
        kDebug(DEBUG_AREA) << "m_should_complete=" << m_should_complete;
        return m_should_complete;
    }
    return false;
}

bool PreprocessorCompletionModel::shouldAbortCompletion(
    KTextEditor::View* const view
  , const KTextEditor::Range& inserted_range
  , const QString& inserted_text
  )
{
    auto* const doc = view->document();                     // get current document
    kDebug(DEBUG_AREA) << "completion text=" << inserted_text;

    if (!m_should_complete)
        return true;

    /// Make sure the current line starts w/ a hash.
    auto text_before = doc->text(
        {KTextEditor::Cursor(inserted_range.end().line(), 0), inserted_range.end()}
      );
    kDebug(DEBUG_AREA) << "text_before=" << text_before;

    /// Check if current line starts w/ \c '#' which is a sign of a preprocessor directive.
    if (text_before[0] != '#')
        return false;
    /// If we are start completion, and user entered text still empty,
    /// when no reason to abort completion.
    if (inserted_text.isEmpty())
        return false;

    /// Otherwise, try to collect indices of matched completion items.
    auto matches = std::vector<int>{};
    matches.reserve(COMPLETIONS.size());
    for (auto i = 0u; i < COMPLETIONS.size(); ++i)
    {
        auto text = COMPLETIONS[i].text;
        const auto end_of_first_word = text.indexOf(' ');
        if (end_of_first_word != -1)
            text = text.left(end_of_first_word);
        const auto is_match = inserted_text.size() < text.size()
          && text.startsWith(inserted_text)
          ;
        if (is_match)
            matches.emplace_back(i);
    }

    /// Then, if matched items count equal to one, that means
    /// we can auto-insert that item (cuz there is no other alternatives).
    if (matches.size() == 1)
    {
        auto item_text = COMPLETIONS[matches[0]].text;
        const auto column = item_text.indexOf('|');
        if (column != -1)
            item_text.remove(column, 1);

        doc->replaceText(inserted_range, item_text);

        // Try to reposition a cursor inside a current view
        if (column != -1)
        {
            auto pos = inserted_range.start();
            pos.setColumn(pos.column() + column);
            doc->activeView()->setCursorPosition(pos);
        }
        kDebug(DEBUG_AREA) << "yEAH! wANt ABrT;";
        return true;
    }
    m_should_complete = !matches.empty();
    kDebug(DEBUG_AREA) << "m_should_complete=" << m_should_complete;
    return !m_should_complete;
}

#if 0
/// \attention It doesn't used actually in kate <= 3.14
bool PreprocessorCompletionModel::shouldExecute(const QModelIndex& index, const QChar inserted)
{
    assert("Sanity check" && index.isValid() && index.row() < COMPLETIONS.size());
    kDebug(DEBUG_AREA) << "selected text:" << COMPLETIONS[index.row()].text;
    return false;
}
#endif

void PreprocessorCompletionModel::completionInvoked(
    KTextEditor::View* const view
  , const KTextEditor::Range& word
  , InvocationType /*itype*/
  )
{
    // Reuse shouldStartCompletion to disable/enable completion
    m_should_complete = shouldStartCompletion(view, QString{}, false, word.end());
}

/**
 * First time this function will be called w/ \e invalid node
 * and a top level group index must be returned (w/ internal
 * \c Level::GROUP identifier). Then it will
 * be called for a group node and increasing \c row index:
 * leaf (item) node index must be returned. Otherwise, returns
 * an \e invalid index.
 */
QModelIndex PreprocessorCompletionModel::index(
    const int row
  , const int column
  , const QModelIndex& parent
  ) const
{
    if (!m_should_complete)                                 // Do nothing if completer disabled
        return QModelIndex{};

    if (!parent.isValid())                                  // Is this invisible root (top level) node?
    {
        assert("Row/column expected to be zero" && row == 0  && column == 0);
        if (row == 0  && column == 0)                       /// \todo Assert this?
            return createIndex(0, 0, Level::GROUP);
        return QModelIndex{};
    }
    if (parent.internalId() == Level::GROUP)
    {
        assert(
            "Row must be less then items count"
            && unsigned(row) < COMPLETIONS.size()
          );
        return createIndex(row, column, Level::ITEM);
    }
    return QModelIndex{};
}

/**
 * This model has the only column! Always!
 * \c returns \c 0 for invalid indices, \c 1 otherwise.
 */
int PreprocessorCompletionModel::columnCount(const QModelIndex& parent) const
{
    return m_should_complete ? int(parent.isValid()) : 0;
}

/**
 * Rows count are the following:
 * - \e invalid (which can be a hidden top level) node is 1 -- i.e. the model has one group
 * - \c 0 for leaf (item nodes) is always zero
 * - remaining node assumed to be a group one, so items count is a number of child rows
 */
int PreprocessorCompletionModel::rowCount(const QModelIndex& parent) const
{
    if (!m_should_complete)                                 // Do nothing if completer disabled
        return 0;

    if (!parent.isValid())
        return 1;
    if (parent.internalId() == Level::ITEM)
        return 0;
    return static_cast<int>(COMPLETIONS.size());
}

/**
 * 
 */
QModelIndex PreprocessorCompletionModel::parent(const QModelIndex& index) const
{
    if (!m_should_complete)                                 // Do nothing if completer disabled
        return QModelIndex{};

    // Return invalid index for top level group and/or other invalid indices
    if (!index.isValid() || index.internalId() == Level::GROUP)
        return QModelIndex{};
    // For item nodes just return an index of group node
    return index.internalId() == Level::ITEM
        ? createIndex(0, 0, Level::GROUP)
        : QModelIndex{}
        ;
}

QVariant PreprocessorCompletionModel::data(const QModelIndex& index, const int role) const
{
    if (!m_should_complete)                                 // Do nothing if completer disabled
        return QVariant{};

    assert("parent expected to be valid" && index.isValid());

    switch (role)
    {
        case HighlightingMethod:
        case MatchQuality:
        case SetMatchContext:
            return QVariant::Invalid;
        case ScopeIndex:
            return -1;
        case InheritanceDepth:
            return index.row() == 0 ? 1 : 100;
        case CodeCompletionModel::GroupRole:
            return Qt::DisplayRole;
        case CompletionRole:
            return QVariant{GlobalScope | LocalScope};
        case ArgumentHintDepth:
            return 0;

        case Qt::DisplayRole:
            switch (index.column())
            {
                case KTextEditor::CodeCompletionModel::Name:
                    if (index.internalId() == Level::ITEM)
                    {
                        assert("Sanity check" && std::size_t(index.row()) < COMPLETIONS.size());
                        auto text = COMPLETIONS[index.row()].text;
                        // Return completion item text w/ cursor indicator removed
                        return text.remove('|');
                    }
                    break;
                case KTextEditor::CodeCompletionModel::Prefix:
                    if (index.internalId() == Level::GROUP)
                        return i18n("Preprocessor");
            }
            break;
        default:
            break;
    }
    return QVariant{};
}

void PreprocessorCompletionModel::executeCompletionItem2(
    KTextEditor::Document* const doc
  , const KTextEditor::Range& word
  , const QModelIndex& index
  ) const
{
    assert("Invalid index is not expected here!" && index.isValid());
    assert("Parent index is not valid" && index.parent().isValid());
    assert("Parent index must be GROUP" && index.parent().internalId() == Level::GROUP);
    assert("Index points to invalid item" && unsigned(index.row()) < COMPLETIONS.size());

    auto text = COMPLETIONS[index.row()].text;
    const auto column = text.indexOf('|');
    if (column != -1)
        text.remove(column, 1);

    doc->replaceText(word, text);

    // Try to reposition a cursor inside a current view
    if (column != -1)
    {
        auto pos = word.start();
        pos.setColumn(pos.column() + column);
        doc->activeView()->setCursorPosition(pos);
    }
}

}                                                           // namespace kate

// kate: hl C++/Qt4;
