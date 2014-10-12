/**
 * \file
 *
 * \brief Class \c kate::IncludeHelperCompletionModel (implementation)
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

// Project specific includes
#include "include_helper_completion_model.h"
#include "cpp_helper_plugin.h"
#include "utils.h"

// Standard includes
#include <KDE/KTextEditor/Document>
#include <KDE/KTextEditor/HighlightInterface>
#include <KDE/KTextEditor/View>
#include <KDE/KLocalizedString>

namespace kate {

//BEGIN IncludeHelperCompletionModel::CompletionItem
IncludeHelperCompletionModel::CompletionItem::CompletionItem(const QString item_text, const bool isDir)
  : text(item_text)
  , is_directory(isDir)
{
}
//END IncludeHelperCompletionModel::CompletionItem

//BEGIN IncludeHelperCompletionModel
IncludeHelperCompletionModel::IncludeHelperCompletionModel(
    QObject* const parent
  , CppHelperPlugin* const plugin
  )
  : KTextEditor::CodeCompletionModel2(parent)
  , m_plugin(plugin)
{
}

//BEGIN KTextEditor::CodeCompletionModelControllerInterface3
/**
 * Initiate completion when there is \c #include on a line (\c range
 * in a result of \c parseIncludeDirective() not empty -- i.e. there is some file present)
 * and cursor placed within that range... despite of completeness of the whole line.
 */
bool IncludeHelperCompletionModel::shouldStartCompletion(
    KTextEditor::View* const view
  , const QString& inserted_text
  , const bool user_insertion
  , const KTextEditor::Cursor& position
  )
{
    kDebug(DEBUG_AREA) << "pos=" << position << ", text=" << inserted_text << ", ui=" << user_insertion;

    m_should_complete = false;

    auto* const doc = view->document();                     // get current document
    auto* const iface = qobject_cast<KTextEditor::HighlightInterface*>(doc);
    // Do nothing if no highlighting interface or not suitable document or
    // a place within it... (we won't to complete smth in non C++ files or comments for example)
    if (!iface || !isSuitableDocumentAndHighlighting(doc->mimeType(), iface->highlightingModeAt(position)))
        return m_should_complete;

    // Try to parse it...
    const auto& line = doc->line(position.line());          // get current line
    auto r = parseIncludeDirective(line, false);
    m_should_complete = r.range.isValid();
    kDebug(DEBUG_AREA) << "m_should_complete=" << m_should_complete;
    if (m_should_complete)
    {
        kDebug(DEBUG_AREA) << "range=" << r.range;
        m_should_complete = position.column() >= r.range.start().column()
          && position.column() <= r.range.end().column();
        if (m_should_complete)
        {
            m_closer = r.close_char();
            kDebug(DEBUG_AREA) << "closer=" << m_closer;
        }
    }
    return m_should_complete;
}

/**
 * We have to stop \c #include completion when current line would parsed well
 * (i.e. contains complete \c #include expression) or have no \c #include at all.
 */
bool IncludeHelperCompletionModel::shouldAbortCompletion(
    KTextEditor::View* const view
  , const KTextEditor::Range& range
  , const QString& current_completion
  )
{
    kDebug(DEBUG_AREA) << "range=" << range << ", current_completion=" << current_completion;
    kDebug(DEBUG_AREA) << "m_should_complete=" << m_should_complete << ", closer=" << m_closer;

    // Get current line
    const auto line = view->document()->line(range.end().line());
    // Try to parse it...
    auto r = parseIncludeDirective(line, false);
    // nothing to complete for lines w/o #include
    const auto need_abort = !r.range.isValid()
      || range.end().column() < r.range.start().column()
      || range.end().column() > (r.range.end().column() + 1)
      ;
    kDebug(DEBUG_AREA) << "result=" << need_abort;
    return need_abort;
}

KTextEditor::Range IncludeHelperCompletionModel::completionRange(
    KTextEditor::View* const view
  , const KTextEditor::Cursor& position
  )
{
    kDebug(DEBUG_AREA) << "cursor: " << position;
    const auto line = view->document()->line(position.line());
    auto r = parseIncludeDirective(line, false);
    if (r.range.isValid())
    {
        auto start = line.lastIndexOf('/', r.range.end().column() - 1);
        kDebug(DEBUG_AREA) << "init start=" << start;
        start = start == -1 ? r.range.start().column() : start + 1;
        kDebug(DEBUG_AREA) << "fixed start=" << start;
        KTextEditor::Range range(
            position.line()
          , start
          , position.line()
          , r.range.end().column()
          );
        kDebug(DEBUG_AREA) << "selected range: " << range;
        return range;
    }
    kDebug(DEBUG_AREA) << "default select";
    return KTextEditor::CodeCompletionModelControllerInterface3::completionRange(view, position);
}
//END KTextEditor::CodeCompletionModelControllerInterface3

//BEGIN KTextEditor::CodeCompletionModel
int IncludeHelperCompletionModel::columnCount(const QModelIndex& parent) const
{
    return int(parent.isValid());
}

int IncludeHelperCompletionModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid())
        return 1;
    if (parent.internalId() == Level::ITEM)
        return 0;
    return static_cast<int>(m_completions.size());
}

QModelIndex IncludeHelperCompletionModel::parent(const QModelIndex& index) const
{
    // Return invalid index for top level group and/or other invalid indices
    if (!index.isValid() || index.internalId() == Level::GROUP)
        return QModelIndex{};
    // For item nodes just return an index of group node
    return index.internalId() == Level::ITEM
        ? createIndex(0, 0, Level::GROUP)
        : QModelIndex{}
        ;
}

QModelIndex IncludeHelperCompletionModel::index(
    const int row
  , const int column
  , const QModelIndex& parent
  ) const
{
    if (!parent.isValid())
    {
#if 0
        assert("Row/column expected to be zero" && row == 0  && column == 0);
#endif
        // At 'top' level only 'header' present, so nothing else than row 0 can be here...
        // As well as nothing for leaf nodes required...
        if (row == 0  && column == 0)                       /// \todo Assert this?
            return createIndex(0, 0, Level::GROUP);
        return QModelIndex{};
    }

    if (parent.internalId() == Level::GROUP)
    {
        assert(
            "Row must be less then items count"
            && unsigned(row) < m_completions.size()
          );
        return createIndex(row, column, Level::ITEM);
    }
    return QModelIndex{};
}

QVariant IncludeHelperCompletionModel::data(const QModelIndex& index, const int role) const
{
    if (!index.isValid() || !m_should_complete)
        return QVariant{};

    switch (role)
    {
        case HighlightingMethod:
        case SetMatchContext:
            return QVariant::Invalid;
#if 0
        case Qt::DecorationRole:
            if (index.column() == KTextEditor::CodeCompletionModel::Icon && !index.parent().isValid())
                return QIcon(KIcon("text-x-c++hdr").pixmap(QSize(16, 16)));
            break;
#endif
        case GroupRole:
            // kDebug(DEBUG_AREA) << "GroupRole";
            return Qt::DisplayRole;

        case Qt::DisplayRole:
            // kDebug(DEBUG_AREA) << "Qt::DisplayRole";
            switch (index.column())
            {
                case KTextEditor::CodeCompletionModel::Name:
                    if (index.internalId() == Level::ITEM)
                    {
                        assert("Sanity check" && std::size_t(index.row()) < m_completions.size());
                        // Return completion item text w/ cursor indicator removed
                        return m_completions[index.row()].text;
                    }
                    break;
                case KTextEditor::CodeCompletionModel::Prefix:
                    // kDebug(DEBUG_AREA) << "Prefix";
                    if (index.internalId() == Level::GROUP)
                        return i18nc("@title:row", "Filesystem");
                    assert("Index check" && std::size_t(index.row()) < m_completions.size());
                    if (m_completions[index.row()].is_directory)
                        return i18nc("@item:inlistbox", "dir");
                    break;
                case KTextEditor::CodeCompletionModel::Scope:
                    // kDebug(DEBUG_AREA) << "Scope";
                    break;
                case KTextEditor::CodeCompletionModel::Arguments:
                    // kDebug(DEBUG_AREA) << "Arguments";
                    break;
                case KTextEditor::CodeCompletionModel::Postfix:
                    // kDebug(DEBUG_AREA) << "Postfix";
                    break;
            }
            break;
        case InheritanceDepth:
            // kDebug(DEBUG_AREA) << "InheritanceDepth";
            return 0;
        case ArgumentHintDepth:
            // kDebug(DEBUG_AREA) << "ArgumentHintDepth";
            return 0;
        case CompletionRole:
            // kDebug(DEBUG_AREA) << "CompletionRole";
            return QVariant{GlobalScope | LocalScope};
        case ItemSelected:
            // kDebug(DEBUG_AREA) << "ItemSelected";
            // return QString("Tooltip text");
            break;
        default:
            break;
    }
    return QVariant{};
}

void IncludeHelperCompletionModel::completionInvoked(
    KTextEditor::View* const view
  , const KTextEditor::Range& range
  , const InvocationType
  )
{
    auto* const doc = view->document();
    kDebug(DEBUG_AREA) << range << ", " << doc->text(range);
    const auto& t = doc->line(range.start().line()).left(range.start().column());
    kDebug(DEBUG_AREA) << "text to parse: " << t;
    auto r = parseIncludeDirective(t, false);
    if (r.range.isValid())
    {
        m_should_complete = range.start().column() >= r.range.start().column()
          && range.start().column() <= r.range.end().column();
        if (m_should_complete)
        {
            r.range.setBothLines(range.start().line());
            kDebug(DEBUG_AREA) << "parsed range: " << r.range;
            m_closer = r.close_char();
            kDebug(DEBUG_AREA) << "doc-path:" << doc->url().directory();
            updateCompletionList(
                doc->text(r.range)
              , r.type == IncludeStyle::local ? doc->url().directory() : QString{}
              );
        }
    }
}

void IncludeHelperCompletionModel::updateCompletionList(const QString& start, const QString& parent_path)
{
    kDebug(DEBUG_AREA) << "IncludeHelper: Form completion list for '" << start << "' [parent-path=" << parent_path << "]";
    beginResetModel();

    m_completions.clear();

    const auto info = QFileInfo{start};
    const auto path = !start.isEmpty() ? info.dir().path() : QString{};
    const auto name = info.completeBaseName() + "*";

    QStringList mask;
    mask.append(name);

    if (parent_path.isEmpty())
    {
        // #include <> needs to be completed
        updateCompletionListPath(
            path
          , m_plugin->config().systemDirs()
          , mask
          , m_plugin->config().ignoreExtensions()
          );
        updateCompletionListPath(
            path
          , m_plugin->config().sessionDirs()
          , mask
          , m_plugin->config().ignoreExtensions()
          );
    }
    if (!parent_path.isEmpty() || m_plugin->config().useCwd())
    {
        // #include "" needs to be completed or "Use current dir is ON"
        QStringList paths;
        paths << parent_path;
        updateCompletionListPath(path, paths, mask, m_plugin->config().ignoreExtensions());
        // Append ".." to completions list for convinience only if #include "" gets completed
        const auto is_root = (
            QDir::rootPath() == QDir{QDir::cleanPath(parent_path)}.filePath(path)
          );
        if (!parent_path.isEmpty() && !is_root)
            m_completions.emplace_back(QString{".."} + QDir::separator(), true);
    }
    endResetModel();
}

void IncludeHelperCompletionModel::executeCompletionItem2(
    KTextEditor::Document* const document
  , const KTextEditor::Range& word
  , const QModelIndex& index
  ) const
{
    kDebug(DEBUG_AREA) << "rword=" << word;
    assert("Index check" && std::size_t(index.row()) < m_completions.size());

    auto p = m_completions[index.row()].text;
    const auto is_file = !m_completions[index.row()].is_directory;
    if (is_file)
    {
        // Get line to be replaced and check if #include has a close char...
        const auto line = document->line(word.start().line());
        const auto r = parseIncludeDirective(line, false);
        if (r.range.isValid() && !r.is_complete)
            p += r.close_char();
    }
    document->replaceText(word, p);

    // Move cursor after the close char if completion item was a file
    auto* const view = document->activeView();
    if (view && is_file)
        view->setCursorPosition({
            word.start().line()
          , word.start().column() + p.length() + 1
          });
}
//END KTextEditor::CodeCompletionModel

void IncludeHelperCompletionModel::updateCompletionListPath(
    const QString& path                                     ///< Path to append to every dir in a list to scan
  , const QStringList& dirs2scan                            ///< List of directories to scan
  , const QStringList& masks                                ///< Filename masks used for globbing
  , const QStringList& ignore_extensions                    ///< Extensions to ignore
  )
{
    kDebug(DEBUG_AREA) << "path =" << path
      << "dirs2scan =" << dirs2scan
      << "masks =" << masks
      << "ignore_extensions =" << ignore_extensions
      ;
    const QDir::Filters glob_flags = QDir::NoDotAndDotDot
      | QDir::AllDirs
      | QDir::Files
      | QDir::CaseSensitive
      | QDir::Readable
      ;
    for (const auto& d : dirs2scan)
    {
        const auto dir = QDir::cleanPath(d + '/' + path);
        kDebug(DEBUG_AREA) << "Trying " << dir;

        for (const auto& info : QDir(dir).entryInfoList(masks, glob_flags))
        {
            if (!info.isDir() && !(info.isFile() && info.isReadable()))
                // Skip everything that is not a directory or readable file
                continue;

            const auto is_directory = info.isDir();
            auto text = info.fileName();
            if (is_directory)
                text += QDir::separator();

            // Make sure that entry not in a list yet
            const auto it = std::find_if(
                begin(m_completions)
              , end(m_completions)
              , [is_directory, &text](const auto& item)
                {
                    return item.text == text && item.is_directory == is_directory;
                }
              );
            if (it == end(m_completions) && !ignore_extensions.contains(info.completeSuffix()))
                m_completions.emplace_back(text, is_directory);
        }
    }
}

//END IncludeHelperCompletionModel
}                                                           // namespace kate
// kate: hl C++/Qt4;
