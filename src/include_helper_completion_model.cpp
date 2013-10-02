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
#include <src/include_helper_completion_model.h>
#include <src/cpp_helper_plugin.h>
#include <src/utils.h>

// Standard includes
#include <KTextEditor/Document>
#include <KTextEditor/HighlightInterface>
#include <KTextEditor/View>
#include <KLocalizedString>                                 /// \todo Where is \c i18n() defiend?

namespace kate {
//BEGIN IncludeHelperCompletionModel
IncludeHelperCompletionModel::IncludeHelperCompletionModel(
    QObject* parent
  , CppHelperPlugin* plugin
  )
  : KTextEditor::CodeCompletionModel2(parent)
  , m_plugin(plugin)
  , m_closer(0)
  , m_should_complete(false)
{
}

QModelIndex IncludeHelperCompletionModel::index(int row, int column, const QModelIndex& parent) const
{
    // kDebug(DEBUG_AREA) << "row=" << row << ", col=" << column << ", p=" << parent.isValid();
    if (!parent.isValid())
    {
        // At 'top' level only 'header' present, so nothing else than row 0 can be here...
        // As well as nothing for leaf nodes required...
        return row == 0 ? createIndex(row, column, 0) : QModelIndex();
    }

    if (parent.parent().isValid())
        return QModelIndex();

    // Check bounds
    const int count = m_dir_completions.size() + m_file_completions.size();
    if (row < count && row >= 0 && column >= 0)
        return createIndex(row, column, 1);                 // make leaf node
    return QModelIndex();
}

/**
 * Initiate completion when there is \c #include on a line (\c m_range
 * in a result of \c parseIncludeDirective() not empty -- i.e. there is some file present)
 * and cursor placed within that range... despite of completeness of the whole line.
 */
bool IncludeHelperCompletionModel::shouldStartCompletion(
    KTextEditor::View* view
  , const QString& inserted_text
  , bool user_insertion
  , const KTextEditor::Cursor& position
  )
{
    kDebug(DEBUG_AREA) << "position=" << position << ", inserted_text=" << inserted_text << ", ui=" << user_insertion;

    m_should_complete = false;
    auto* doc = view->document();                           // get current document
    auto line = doc->line(position.line());                 // get current line
    KTextEditor::HighlightInterface* iface = qobject_cast<KTextEditor::HighlightInterface*>(doc);
    // Do nothing if no highlighting interface or not suitable document or
    // a place within it... (we won't to complete smth in non C++ files or comments for example)
    if (!iface || !isSuitableDocumentAndHighlighting(doc->mimeType(), iface->highlightingModeAt(position)))
        return m_should_complete;

    // Try to parse it...
    IncludeParseResult r = parseIncludeDirective(line, false);
    m_should_complete = r.m_range.isValid();
    if (m_should_complete)
    {
        kDebug(DEBUG_AREA) << "range=" << r.m_range;
        m_should_complete = position.column() >= r.m_range.start().column()
          && position.column() <= r.m_range.end().column();
        if (m_should_complete)
        {
            m_closer = r.close_char();
            kDebug(DEBUG_AREA) << "closer=" << m_closer;
        }
    }
    else if (position.column() == line.length())
    {
        QString text = tryToCompleteIncludeDirective(line.mid(0, position.column()).trimmed());
        m_should_complete = !text.isEmpty();
        if (m_should_complete)
        {
            /// \todo Hardcoded angle bracket! Better to check what file was selected
            /// (from system path or session specific) and replace it accordingly...
            text += QLatin1String(" <");
            auto start = position;
            start.setColumn(0);
            KTextEditor::Range range = {start, position};
            view->document()->replaceText(range, text);
        }
    }
    return m_should_complete;
}

/**
 * We have to stop \c #include completion when current line would parsed well
 * (i.e. contains complete \c #include expression) or have no \c #include at all.
 */
bool IncludeHelperCompletionModel::shouldAbortCompletion(
    KTextEditor::View* view
  , const KTextEditor::Range& range
  , const QString& current_completion
  )
{
    kDebug(DEBUG_AREA) << "range=" << range << ", current_completion=" << current_completion;
    kDebug(DEBUG_AREA) << "m_should_complete=" << m_should_complete << ", closer=" << m_closer;

    // Get current line
    QString line = view->document()->line(range.end().line());
    // Try to parse it...
    IncludeParseResult r = parseIncludeDirective(line, false);
    // nothing to complete for lines w/o #include
    const bool need_abort = !r.m_range.isValid()
      || range.end().column() < r.m_range.start().column()
      || range.end().column() > (r.m_range.end().column() + 1)
      ;
    kDebug(DEBUG_AREA) << "result=" << need_abort;
    return need_abort;
}

void IncludeHelperCompletionModel::completionInvoked(
    KTextEditor::View* view
  , const KTextEditor::Range& range
  , InvocationType
  )
{
    KTextEditor::Document* doc = view->document();
    kDebug(DEBUG_AREA) << range << ", " << doc->text(range);
    const QString& t = doc->line(range.start().line()).left(range.start().column());
    kDebug(DEBUG_AREA) << "text to parse: " << t;
    IncludeParseResult r = parseIncludeDirective(t, false);
    if (r.m_range.isValid())
    {
        m_should_complete = range.start().column() >= r.m_range.start().column()
            && range.start().column() <= r.m_range.end().column();
        if (m_should_complete)
        {
            r.m_range.setBothLines(range.start().line());
            kDebug(DEBUG_AREA) << "parsed range: " << r.m_range;
            m_closer = r.close_char();
            updateCompletionList(doc->text(r.m_range), r.m_type == IncludeStyle::local);
        }
    }
}

void IncludeHelperCompletionModel::updateCompletionList(const QString& start, const bool only_local)
{
    kDebug(DEBUG_AREA) << "IncludeHelper: Form completion list for " << start;
    beginResetModel();
    m_file_completions.clear();
    m_dir_completions.clear();
    const int slash = start.lastIndexOf('/');
    const QString path = slash != -1 ? start.left(slash) : QString();
    const QString name = slash != -1 ? start.mid(slash + 1) + "*" : QString("*");
    QStringList mask;
    mask.append(name);
    kDebug(DEBUG_AREA) << "mask=" << mask;
    // Complete session dirst first
    updateListsFromFS(
        path
      , m_plugin->config().sessionDirs()
      , mask
      , m_dir_completions
      , m_file_completions
      , m_plugin->config().ignoreExtensions()
      );
    if (!only_local)
    {
        // Complete global dirs next
        updateListsFromFS(
            path
          , m_plugin->config().systemDirs()
          , mask
          , m_dir_completions
          , m_file_completions
          , m_plugin->config().ignoreExtensions()
          );
    }
    //
    kDebug(DEBUG_AREA) << "Got file completions: " << m_file_completions;
    kDebug(DEBUG_AREA) << "Got dir completions: " << m_dir_completions;
    endResetModel();
}

QVariant IncludeHelperCompletionModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || !m_should_complete)
        return QVariant();

#if 0
    kDebug(DEBUG_AREA) << index.isValid() << '/' << index.parent().isValid() << ": index " << index.row()
      << "," << index.column() << ", role " << role;
#endif

    switch (role)
    {
        case HighlightingMethod:
        case SetMatchContext:
            return QVariant::Invalid;
        case GroupRole:
            // kDebug(DEBUG_AREA) << "GroupRole";
            return Qt::DisplayRole;
#if 0
        case Qt::DecorationRole:
            if (index.column() == KTextEditor::CodeCompletionModel::Icon && !index.parent().isValid())
                return QIcon(KIcon("text-x-c++hdr").pixmap(QSize(16, 16)));
            break;
#endif
        case Qt::DisplayRole:
            // kDebug(DEBUG_AREA) << "Qt::DisplayRole";
            switch (index.column())
            {
                case KTextEditor::CodeCompletionModel::Name:
                    // kDebug(DEBUG_AREA) << "Name";
                    if (index.parent().isValid())
                        return (index.row() < m_dir_completions.size())
                          ? m_dir_completions[index.row()]
                          : m_file_completions[index.row() - m_dir_completions.size()];
                    break;
                case KTextEditor::CodeCompletionModel::Prefix:
                    // kDebug(DEBUG_AREA) << "Prefix";
                    if (!index.parent().isValid())
                        return i18n("Include Helper");
                    if (index.row() < m_dir_completions.size())
                        return i18n("dir");
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
            return QVariant(GlobalScope | LocalScope);
        case ItemSelected:
            // kDebug(DEBUG_AREA) << "ItemSelected";
//             return QString("Tooltip text");
        default:
            break;
    }
    return QVariant();
}

void IncludeHelperCompletionModel::executeCompletionItem2(
    KTextEditor::Document* document
  , const KTextEditor::Range& word
  , const QModelIndex& index
  ) const
{
    kDebug(DEBUG_AREA) << "rword=" << word;
    QString p = index.row() < m_dir_completions.size()
      ? m_dir_completions[index.row()]
      : m_file_completions[index.row() - m_dir_completions.size()]
      ;
    kDebug(DEBUG_AREA) << "dict=" << p;
    if (!p.endsWith("/"))                                   // Is that dir or file completion?
    {
        // Get line to be replaced and check if #include hase close char...
        QString line = document->line(word.start().line());
        IncludeParseResult r = parseIncludeDirective(line, false);
        if (r.m_range.isValid() && !r.m_is_complete)
            p += r.close_char();
    }
    document->replaceText(word, p);
}

KTextEditor::Range IncludeHelperCompletionModel::completionRange(
    KTextEditor::View* view
  , const KTextEditor::Cursor& position
  )
{
    kDebug(DEBUG_AREA) << "cursor: " << position;
    QString line = view->document()->line(position.line());
    IncludeParseResult r = parseIncludeDirective(line, false);
    if (r.m_range.isValid())
    {
        int start = line.lastIndexOf('/', r.m_range.end().column() - 1);
        kDebug(DEBUG_AREA) << "init start=" << start;
        start = start == -1 ? r.m_range.start().column() : start + 1;
        kDebug(DEBUG_AREA) << "fixed start=" << start;
        KTextEditor::Range range(
            position.line()
          , start
          , position.line()
          , r.m_range.end().column()
          );
        kDebug(DEBUG_AREA) << "selected range: " << range;
        return range;
    }
    kDebug(DEBUG_AREA) << "default select";
#if 0
    return KTextEditor::Range();
#else
    return KTextEditor::CodeCompletionModelControllerInterface3::completionRange(view, position);
#endif
}
//END IncludeHelperCompletionModel
}                                                           // namespace kate
// kate: hl C++11/Qt4;
