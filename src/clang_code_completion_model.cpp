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
#include <src/cpp_helper_plugin.h>
#include <src/document_proxy.h>
#include <src/diagnostic_messages_model.h>
#include <src/translation_unit.h>
#include <src/utils.h>

// Standard includes
#include <KTextEditor/Document>
#include <KTextEditor/HighlightInterface>
#include <KTextEditor/TemplateInterface>
#include <KTextEditor/View>
#include <KLocalizedString>                                 /// \todo Where is \c i18n() defiend?
#include <algorithm>

namespace kate {

ClangCodeCompletionModel::ClangCodeCompletionModel(
    QObject* parent
  , CppHelperPlugin* plugin
  , DiagnosticMessagesModel& dmm
  )
  : KTextEditor::CodeCompletionModel2(parent)
  , m_plugin(plugin)
  , m_diagnostic_model(dmm)
  , m_current_view(nullptr)
{
}

bool ClangCodeCompletionModel::shouldStartCompletion(
    KTextEditor::View* view
  , const QString& inserted_text
  , bool user_insertion
  , const KTextEditor::Cursor& pos
  )
{
    auto* doc = view->document();                           // get current document
    bool result = false;
    KTextEditor::HighlightInterface* iface = qobject_cast<KTextEditor::HighlightInterface*>(doc);
    if (iface)
    {
        kDebug(DEBUG_AREA) << "higlighting mode at" << pos << ':' << iface->highlightingModeAt(pos);
        bool is_completion_needed = user_insertion
          && m_plugin->config().autoCompletions()
          && isSuitableDocumentAndHighlighting(doc->mimeType(), iface->highlightingModeAt(pos))
          ;
        if (is_completion_needed)
        {
            auto text = inserted_text.trimmed();
            result = text.endsWith(QLatin1String(".")) || text.endsWith(QLatin1String("->"));
        }
    }
    kDebug(DEBUG_AREA) << "result:" << result;
    return result;
}

void ClangCodeCompletionModel::completionInvoked(
    KTextEditor::View* view
  , const KTextEditor::Range& range
  , InvocationType /*invocationType*/
  )
{
    m_current_view = view;
    KTextEditor::Document* doc = view->document();
    KUrl url = doc->url();
    if (!url.isValid() || url.isEmpty())
    {
        /// \todo Turn into a popup
        kWarning() << "U have to have a document on a disk before use code completion";
        return;
    }
    kDebug(DEBUG_AREA) << "Comletion requested at " << range << "for" << doc->text(range);

    // Remove everything collected before
    m_groups.clear();

    try
    {
        auto& unit = m_plugin->getTranslationUnitByDocument(doc);
        // Show some SPAM in a tool view
        m_diagnostic_model.append(
            DiagnosticMessagesModel::Record(
                doc->url().toLocalFile()
              , range.start().line() + 1
              , range.start().column() + 1
              , "Completion point"
              , DiagnosticMessagesModel::Record::type::debug
              )
          );
        // Obtain diagnostic if any
        {
            auto diag = unit.getLastDiagnostic();
            if (!diag.empty())
                m_diagnostic_model.append(
                    std::make_move_iterator(begin(diag))
                  , std::make_move_iterator(end(diag))
                  );
        }
        // Try to make a completion
        auto completions = unit.completeAt(
            unsigned(range.start().line() + 1)              // NOTE Kate count lines starting from 0
          , unsigned(range.start().column() + 1)            // NOTE Kate count columns starting from 0
          , m_plugin->config().completionFlags()
          , m_plugin->config().sanitizeCompletions()
              ? m_plugin->config().sanitizeRules()
              : PluginConfiguration::sanitize_rules_list_type()
          );
        // Obtain diagnostic if any
        {
            auto diag = unit.getLastDiagnostic();
            if (!diag.empty())
                m_diagnostic_model.append(
                    std::make_move_iterator(begin(diag))
                  , std::make_move_iterator(end(diag))
                  );
        }

        // Transform a plain list into hierarchy grouped by a parent context
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
          , [](std::pair<const QString, GroupInfo>&& p) { return std::move(p); }
          );
    }
    catch (const TranslationUnit::Exception& e)
    {
        m_diagnostic_model.append(
            DiagnosticMessagesModel::Record(
                QString("Fail to make a code completion: %1").arg(e.what())
              , DiagnosticMessagesModel::Record::type::error
              )
          );
    }
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
    result = m_groups[index.internalId()].second.m_completions[index.row()].data(
        index
      , role
      , m_plugin->config().usePrefixColumn()
      );
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
                    kDebug(DEBUG_AREA) << "OK: HighlightingMethod(" << role << ") called for" << index;
#endif
                    return KTextEditor::CodeCompletionModel::CustomHighlighting;
                default:
#if 0
                    kDebug(DEBUG_AREA) << "HighlightingMethod(" << role << ") DEFAULT called for " << index;
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
                    kDebug(DEBUG_AREA) << "OK: CustomHighlight(" << role << ") called for" << index;
#endif
                    // Request item for text to highlight
                    auto text = m_groups[index.internalId()]
                      .second.m_completions[index.row()]
                      .data(index, Qt::DisplayRole, m_plugin->config().usePrefixColumn())
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
                    kDebug(DEBUG_AREA) << "CustomHighlight(" << role << ") DEFAULT called for" << index;
#endif
                    break;                                  // Return nothing
            }
            break;                                          // Return nothing
    }
    return QVariant();
}

void ClangCodeCompletionModel::executeCompletionItem2(
    KTextEditor::Document* doc
  , const KTextEditor::Range& word
  , const QModelIndex& index
  ) const
{
    assert("Active view expected to be equal to the stored one" && doc->activeView() == m_current_view);
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

    KTextEditor::TemplateInterface* template_iface =
        qobject_cast<KTextEditor::TemplateInterface*>(m_current_view);
    if (template_iface)
    {
        kDebug(DEBUG_AREA) << "TemplateInterface available for a view" << m_current_view;
        ClangCodeCompletionItem::CompletionTemplateData result = m_groups[index.internalId()]
          .second.m_completions[index.row()]
          .getCompletionTemplate();
        kDebug(DEBUG_AREA) << "Template:" << result.m_tpl;
        kDebug(DEBUG_AREA) << "Values:" << result.m_values;
        // Check if current template is a function and there is a '()' right after cursor
        auto range = word;
        if (result.m_is_function)
        {
            auto next_word_range = DocumentProxy(doc).firstWordAfterCursor(word.end());
            kDebug(DEBUG_AREA) << "OK THIS IS FUNCTION TEMPLATE: next word range" << next_word_range;
            kDebug(DEBUG_AREA) << "replace range before:" << range;
            if (next_word_range.isValid() && doc->text(next_word_range).startsWith(QLatin1String("()")))
            {
                range.end().setColumn(next_word_range.start().column() + 2);
                kDebug(DEBUG_AREA) << "replace range after:" << range;
            }
        }
        doc->removeText(range);
        template_iface->insertTemplateText(range.start(), result.m_tpl, result.m_values);
    }
    else
    {
        kDebug(DEBUG_AREA) << "No TemplateInterface for a view" << m_current_view;
        auto p = m_groups[index.internalId()].second.m_completions[index.row()].executeCompletion();
        doc->replaceText(word, p.first);
        // Try to reposition a cursor inside a current (hope it still is) view
        auto pos = word.start();
        pos.setColumn(pos.column() + p.second);
        m_current_view->setCursorPosition(pos);
    }
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
