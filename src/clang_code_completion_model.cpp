/**
 * \file
 *
 * \brief Class \c kate::ClangCodeCompletionModel (implementation)
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

// Project specific includes
#include <src/clang_code_completion_model.h>
#include <src/clang_utils.h>
#include <src/translation_unit.h>
#include <src/cpp_helper_plugin.h>
#include <src/sanitize_snippet.h>

// Standard includes
#include <ktexteditor/highlightinterface.h>
#include <KTextEditor/Document>
#include <KTextEditor/View>
#include <KLocalizedString>                                 /// \todo Where is \c i18n() defiend?
#include <algorithm>

namespace kate {

ClangCodeCompletionModel::ClangCodeCompletionModel(
    QObject* parent
  , CppHelperPlugin* plugin
  , KTextEdit* dt
  )
  : KTextEditor::CodeCompletionModel2(parent)
  , m_plugin(plugin)
  , m_diagnostic_text(dt)
  , m_current_view(nullptr)
{
    connect(
        &m_plugin->config()
      , SIGNAL(clangOptionsChanged())
      , this
      , SLOT(invalidateTranslationUnit())
      );
}

void ClangCodeCompletionModel::invalidateTranslationUnit()
{
    kDebug() << "Clang options had changed, invalidating translation unit...";
    m_unit.reset();
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

    m_current_view = view;
    KTextEditor::Document* doc = view->document();
    KUrl url = doc->url();
    if (!url.isValid() || url.isEmpty())
    {
        /// \todo Turn into a popup
        kWarning() << "U have to have a document on a disk before use code completion";
        return;
    }

    kDebug() << "It seems user has invoked comletion at " << range;

    // Obtain a completion data for given document or append a new entry
    if (!m_unit)
    {
        // Form command line parameters
        //  1) collect configured system and session dirs and make -I option series
        QStringList options = m_plugin->config().formCompilerOptions();
        //  2) append PCH options
        if (!m_plugin->config().pchFile().isEmpty())
            options << /*"-Xclang" << */"-include-pch" << m_plugin->config().pchFile().toLocalFile();
        // Form unsaved files list
        TranslationUnit::unsaved_files_list_type unsaved_files = makeUnsavedFilesList(doc);

        try
        {
            // Parse it!
            m_unit.reset(
                new TranslationUnit(
                    m_plugin->index()
                  , doc->url()
                  , options
                  , TranslationUnit::defaultEditingParseOptions()
                  , unsaved_files
                  )
              );
        }
        catch (const TranslationUnit::Exception& e)
        {
            kError() << "Fail to make a translation unit: " << e.what();
            /// \todo Show popup?
            return;
        }
    }
    else
    {
        /// \todo It would be better to monitor if code has changed before
        /// \c updateUnsavedFiles() and \c reparse()
        m_unit->updateUnsavedFiles(makeUnsavedFilesList(doc));
#if 0
        m_unit->reparse();                                  // Reparse translation unit
#endif
    }

    // Remove everything collected before
    m_groups.clear();

    try
    {
        // Try to make a comletion
        auto completions = m_unit->completeAt(
            unsigned(range.start().line() + 1)              // NOTE Kate count lines starting from 0
          , unsigned(range.start().column() + 1)            // NOTE Kate count columns starting from 0
          );
        m_diagnostic_text->setText(m_unit->getLastDiagnostic());

        // Transform a plain list into hierarchy grouped by parent context
        std::map<QString, GroupInfo> grouped_completions;
        for (auto&& comp : completions)
        {
            // Find a group for current item
            auto it = grouped_completions.find(comp.parentText());
            if (it == end(grouped_completions))
            {
                // No group yet, let create a new one
                it = grouped_completions.insert(std::make_pair(comp.parentText(), GroupInfo())).first;
            }
            // Add a current item to the list of completions in the current group
            it->second.m_completions.emplace_back(std::move(comp));
        }
        // Convert the collected map to a vector
        m_groups.reserve(grouped_completions.size());
        std::transform(
            std::make_move_iterator(begin(grouped_completions))
          , std::make_move_iterator(end(grouped_completions))
          , std::back_inserter(m_groups)
          , [](decltype(grouped_completions)::value_type&& p) { return std::move(p); }
          );
    }
    catch (const TranslationUnit::Exception& e)
    {
        kError() << "Fail to make a code completion: " << e.what();
    }
}

TranslationUnit::unsaved_files_list_type ClangCodeCompletionModel::makeUnsavedFilesList(
    KTextEditor::Document* doc
  )
{
    // Form unsaved files list
    TranslationUnit::unsaved_files_list_type unsaved_files;
    //  1) append this document to the list of unsaved files
    const QString this_filename = doc->url().toLocalFile();
    unsaved_files.push_back(qMakePair(this_filename, doc->text()));
    /// \todo Collect other unsaved files
    return unsaved_files;
}

int ClangCodeCompletionModel::columnCount(const QModelIndex& /*index*/) const
{
    return 1;
}

int ClangCodeCompletionModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid())                                  // Return groups count for invalid node
        return m_groups.size();
    if (parent.internalId() == Level::GROUP)                // Check if given parent is level1 index
        // ... return count of completion items in a group
        return m_groups[parent.row()].second.m_completions.size();
    return 0;                                               // Otherwise return 0, cuz leaf nodes have no children
}

/**
 * \return
 * \li For level 1 nodes (i.e. groups), return an invisible root node.
 * \li For level 2 nodes, return a corresponding group node. It that case
 * \c internalId() is an index of a parent group.
 */
QModelIndex ClangCodeCompletionModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())                                   // Return an invalid index if given index is a root
        return QModelIndex();
    if (index.internalId() == Level::GROUP)                 // Is the given index is a level1 index?
        return QModelIndex();
    // Otherwise this is level2 index: return a group index
    assert(
        "index.internalId() supposed to be less than m_groups.size()"
      && unsigned(index.internalId()) < m_groups.size()
      );
    return createIndex(index.internalId(), 0, Level::GROUP);
}

QModelIndex ClangCodeCompletionModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!parent.isValid())                                  // Is invisible root node?
    {
        if (0 <= row && unsigned(row) < m_groups.size())    /// \todo use assert instead of runtime check?
            return createIndex(row, column, Level::GROUP);
        return QModelIndex();
    }
    if (parent.internalId() == Level::GROUP)
    {
        assert(
            "row must be less then items count"
          && unsigned(row) < m_groups[parent.row()].second.m_completions.size()
          );
        return createIndex(row, column, parent.row());
    }
    // Leaf nodes have no children
    return QModelIndex();
}

QVariant ClangCodeCompletionModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();                                  // Return nothing for invalid index

    if (index.internalId() == Level::GROUP)
        return getGroupData(index, role);

    // Ok, only leaf nodes here...
    return getItemData(index, role);
}

QVariant ClangCodeCompletionModel::getGroupData(const QModelIndex& index, int role) const
{
    switch (role)
    {
        case KTextEditor::CodeCompletionModel::SetMatchContext:
        case KTextEditor::CodeCompletionModel::MatchQuality:
        case KTextEditor::CodeCompletionModel::HighlightingMethod:
            return QVariant(QVariant::Invalid);
        case KTextEditor::CodeCompletionModel::ScopeIndex:
            return -1;
        case KTextEditor::CodeCompletionModel::InheritanceDepth:
            return QVariant(0);
        case KTextEditor::CodeCompletionModel::GroupRole:
            return QVariant(Qt::DisplayRole);
        case Qt::DisplayRole:
            assert(
                "row must be less than groups size"
              && unsigned(index.row()) < m_groups.size()
              );
            return QVariant(m_groups[index.row()].first);
        default:
            break;
    }
    return QVariant();
}

QVariant ClangCodeCompletionModel::getItemData(const QModelIndex& index, int role) const
{
    assert(
        "ref to parent group is invalid!"
      && unsigned(index.internalId()) < m_groups.size()
      );
    assert(
        "row must be less then completion items in a selected group"
      && unsigned(index.row()) < m_groups[index.internalId()].second.m_completions.size()
      );

    QVariant result;
    if (m_plugin->config().highlightCompletions())
        result = getItemHighlightData(index, role);         // Try to get highlighting info if requested
    if (!result.isNull())                                   // Return any non empty value
        return result;

    // ... try to ask an item otherwise
    result = m_groups[index.internalId()].second.m_completions[index.row()].data(index, role);
    // Check if completion items sanitize requested
    if (m_plugin->config().sanitizeCompletions())
    {
        switch (role)
        {
            case Qt::DisplayRole:
                if (index.column() == CodeCompletionModel2::Prefix)
                    result = sanitizePrefix(result.toString());
                else if (index.column() == CodeCompletionModel2::Arguments)
                    result = sanitizeParams(result.toString());
            default:
                break;
        }
    }
    return result;
}

QVariant ClangCodeCompletionModel::getItemHighlightData(const QModelIndex& index, int role) const
{
    switch (role)
    {
        case KTextEditor::CodeCompletionModel::SetMatchContext:
        case KTextEditor::CodeCompletionModel::MatchQuality:
            return QVariant(QVariant::Invalid);
        case KTextEditor::CodeCompletionModel::ScopeIndex:
            return -1;
        case KTextEditor::CodeCompletionModel::HighlightingMethod:
            // We want custom highlighting for return value, name and params
            switch (index.column())
            {
                case KTextEditor::CodeCompletionModel::Name:
                case KTextEditor::CodeCompletionModel::Prefix:
                case KTextEditor::CodeCompletionModel::Arguments:
#if 0
                    kDebug() << "OK: HighlightingMethod(" << role << ") called for" << index;
#endif
                    return KTextEditor::CodeCompletionModel::CustomHighlighting;
                default:
#if 0
                    kDebug() << "HighlightingMethod(" << role << ") DEFAULT called for " << index;
#endif
                    return QVariant(QVariant::Invalid);
            }
            break;
        case KTextEditor::CodeCompletionModel::CustomHighlight:
            // Highlight completion snippet using hidden document of the plugin instance
            switch (index.column())
            {
                case KTextEditor::CodeCompletionModel::Name:
                case KTextEditor::CodeCompletionModel::Prefix:
                case KTextEditor::CodeCompletionModel::Arguments:
                {
#if 0
                    kDebug() << "OK: CustomHighlight(" << role << ") called for" << index;
#endif
                    // Request item for text to highlight
                    auto text = m_groups[index.internalId()]
                      .second.m_completions[index.row()]
                      .data(index, Qt::DisplayRole)
                      .toString()
                      ;
                    // Do not waste time if nothing to highlight
                    if (text.isEmpty())
                        return QVariant();
                    // Ask the last visited document about current highlighting mode
                    KTextEditor::Document* doc = m_current_view->document();
                    auto mode = doc->highlightingMode();
                    // Colorise a snippet
                    auto attrs = m_plugin->highlightSnippet(text, mode);
                    if (!attrs.isEmpty())
                    {
                        // Transform obtained attribute blocks into a sequence
                        // of variant triplets.
                        QList<QVariant> attributes;
                        for (const auto& a : attrs)
                            attributes
                            << a.start
                            << (a.start + a.length)
                            << KTextEditor::Attribute(*a.attribute)
                            ;
                        return attributes;
                    }
                    // NOTE Fallback to default case...
                }
                default:
#if 0
                    kDebug() << "CustomHighlight(" << role << ") DEFAULT called for" << index;
#endif
                    break;                                  // Return nothing
            }
            break;                                          // Return nothing
    }
    return QVariant();
}

void ClangCodeCompletionModel::executeCompletionItem2(
    KTextEditor::Document* document
  , const KTextEditor::Range& word
  , const QModelIndex& index
  ) const
{
    assert("Invalid index is not expected here!" && index.isValid());
    assert("Parent index is not valid" && index.parent().isValid());
    assert("Parent index must be GROUP" && index.parent().internalId() == Level::GROUP);
    assert(
        "Parent index points to invalid group"
      && 0 <= index.internalId()
      && unsigned(index.internalId()) < m_groups.size()
      );
    assert(
        "Index points to invalid item"
      && 0 <= index.row()
      && unsigned(index.row()) < m_groups[index.internalId()].second.m_completions.size()
      );
    auto p = m_groups[index.internalId()].second.m_completions[index.row()].executeCompletion();
    document->replaceText(word, p.first);
    // Try to reposition a cursor inside a current (hope it still is) view
    for (auto* view : document->views())
    {
        if (view == m_current_view)
        {
            kDebug() << "Found current view: " << view;
            kDebug() << "p = " << p;
            auto pos = word.start();
            kDebug() << "pos = " << pos;
            pos.setColumn(pos.column() + p.second);
            view->setCursorPosition(pos);
        }
    }
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
