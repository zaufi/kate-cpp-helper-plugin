/**
 * \file
 *
 * \brief Class \c kate::include_helper_plugin_complation_model (implementation)
 *
 * \date Mon Feb  6 06:12:41 MSK 2012 -- Initial design
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

// Project specific includes
#include <src/include_helper_plugin_completion_model.h>
#include <src/include_helper_plugin.h>
#include <src/utils.h>
#include <KLocalizedString>                                 /// \todo Where is \c i18n() defiend?

// Standard includes
#include <KTextEditor/Document>
#include <KTextEditor/View>
#include <QDir>

namespace kate {
//BEGIN IncludeHelperPluginCompletionModel
IncludeHelperPluginCompletionModel::IncludeHelperPluginCompletionModel(
    QObject* parent
  , IncludeHelperPlugin* plugin
  )
  : KTextEditor::CodeCompletionModel2(parent)
  , m_plugin(plugin)
  , m_closer(0)
  , m_should_complete(false)
{
}

/// \todo More effective implementation could be here!
QModelIndex IncludeHelperPluginCompletionModel::index(int row, int column, const QModelIndex& parent) const
{
//     kDebug() << "row=" << row << ", col=" << column << ", p=" << parent.isValid();
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

bool IncludeHelperPluginCompletionModel::shouldStartCompletion(
    KTextEditor::View* view
  , const QString& inserted_text
  , bool user_insertion
  , const KTextEditor::Cursor& position
  )
{
    kDebug() << "position=" << position << ", inserted_text=" << inserted_text << ", ui=" << user_insertion;
    QString left_text = view->document()->line(position.line()).left(position.column());
    KTextEditor::Range r = kate::parseIncludeDirective(left_text, false);
    kDebug() << "got range: " << r;
    m_should_complete = !r.isEmpty();
    if (m_should_complete)
        return KTextEditor::CodeCompletionModelControllerInterface3::shouldStartCompletion(
            view, inserted_text, user_insertion, position
          );
    r.setBothLines(position.line());
    m_closer = left_text[r.start().column() - 1];
    kDebug() << "closer=" << m_closer;
    return m_should_complete;
}

bool IncludeHelperPluginCompletionModel::shouldAbortCompletion(
    KTextEditor::View* view
  , const KTextEditor::Range& range
  , const QString& current_completion
  )
{
    kDebug() << "range=" << range << ", current_completion=" << current_completion;
    kDebug() << "m_should_complete=" << m_should_complete << ", closer=" << m_closer;
    kDebug() << "last=" << current_completion[current_completion.length() - 1];
    if (m_should_complete)
    {
        const bool done = !current_completion.isEmpty()
          && current_completion[current_completion.length() - 1] == m_closer;
        if (done)
        {
            m_should_complete = false;
            m_dir_completions.clear();
            m_file_completions.clear();
            m_closer = 0;
        }
        kDebug() << "done=" << done;
        return done;
    }
    return KTextEditor::CodeCompletionModelControllerInterface3::shouldAbortCompletion(
        view, range, current_completion
      );
#if 0
    const QString& text = view->document()->text(range);
    kDebug() << "range text: " << text;
    const QChar last = text.right(1)[0];
    const bool r = last == '>'
      || last == '"'
      ||
    if (r)
    {
        m_should_complete = false;
        m_dir_completions.clear();
        m_file_completions.clear();
        m_closer = 0;
    }
    return r;
#endif
}

void IncludeHelperPluginCompletionModel::completionInvoked(
    KTextEditor::View* view
  , const KTextEditor::Range& range
  , InvocationType invocationType
  )
{
    if (invocationType != KTextEditor::CodeCompletionModel2::AutomaticInvocation || m_should_complete)
    {
        KTextEditor::Document* doc = view->document();
        kDebug() << range << ", " << doc->text(range);
        const QString& t = doc->line(range.start().line()).left(range.start().column());
        kDebug() << "text to parse: " << t;
        KTextEditor::Range r = kate::parseIncludeDirective(t, false);
        r.setBothLines(range.start().line());
        kDebug() << "parsed range: " << r;
        updateCompletionList(doc->text(r));
    }
}

void IncludeHelperPluginCompletionModel::updateCompletionList(const QString& start)
{
    kDebug() << "IncludeHelper: Form completion list for " << start;
    beginResetModel();
    m_file_completions.clear();
    m_dir_completions.clear();
    const int slash = start.lastIndexOf('/');
    const QString path = slash != -1 ? start.left(slash) : QString();
    const QString name = slash != -1 ? start.mid(slash + 1) + "*" : QString("*");
    QStringList mask;
    mask.append(name);
    kDebug() << "mask=" << mask;
    // Complete session dirst first
    Q_FOREACH(const QString& d, m_plugin->sessionDirs())
    {
        const QString dir = QDir::cleanPath(d + '/' + path);
        kDebug() << "Trying " << dir;
        {
            QStringList result = QDir(dir).entryList(
                mask
              , QDir::Dirs | QDir::NoDotAndDotDot | QDir::CaseSensitive | QDir::Readable
              , QDir::Name
              );
            Q_FOREACH(const QString& r, result)
                m_dir_completions.append(r + "/");
        }
        {
            QStringList result = QDir(dir).entryList(
                mask
              , QDir::Files | QDir::NoDotAndDotDot | QDir::CaseSensitive | QDir::Readable
              , QDir::Name
              );
            m_file_completions.append(result);
        }
    }
    // Complete global dirs next
    Q_FOREACH(const QString& d, m_plugin->globalDirs())
    {
        const QString dir = QDir::cleanPath(d + '/' + path);
        kDebug() << "Trying " << dir;
        {
            QStringList result = QDir(dir).entryList(
                mask
              , QDir::Dirs | QDir::NoDotAndDotDot | QDir::CaseSensitive | QDir::Readable
              , QDir::Name
              );
            Q_FOREACH(const QString& r, result)
                m_dir_completions.append(r + "/");
        }
        {
            QStringList result = QDir(dir).entryList(
                mask
              , QDir::Files | QDir::NoDotAndDotDot | QDir::CaseSensitive | QDir::Readable
              , QDir::Name
              );
            m_file_completions.append(result);
        }
    }
    kDebug() << "Got file completions: " << m_file_completions;
    kDebug() << "Got dir completions: " << m_dir_completions;
    endResetModel();
}

QVariant IncludeHelperPluginCompletionModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || !m_should_complete)
        return QVariant();

#if 0
    kDebug() << index.isValid() << '/' << index.parent().isValid() << ": index " << index.row()
      << "," << index.column() << ", role " << role;
#endif

    static QIcon icon(KIcon ("text-x-c++hdr").pixmap(QSize(16, 16)));
    switch (role)
    {
        case HighlightingMethod:
        case SetMatchContext:
            return QVariant::Invalid;
        case GroupRole:
//             kDebug() << "GroupRole";
            return Qt::DisplayRole;
        case Qt::DisplayRole:
//             kDebug() << "Qt::DisplayRole";
            switch (index.column())
            {
                case KTextEditor::CodeCompletionModel::Icon:
//                     kDebug() << "Icon";
                    if (index.parent().isValid())
                        return icon;
                    break;
                case KTextEditor::CodeCompletionModel::Name:
//                     kDebug() << "Name";
                    if (index.parent().isValid())
                        return (index.row() < m_dir_completions.size())
                          ? m_dir_completions[index.row()]
                          : m_file_completions[index.row() - m_dir_completions.size()];
                    break;
                case KTextEditor::CodeCompletionModel::Prefix:
//                     kDebug() << "Prefix";
                    if (!index.parent().isValid())
                        return i18n("Include Helper");
                case KTextEditor::CodeCompletionModel::Scope:
//                     kDebug() << "Scope";
                    break;
                case KTextEditor::CodeCompletionModel::Arguments:
//                     kDebug() << "Arguments";
                    break;
                case KTextEditor::CodeCompletionModel::Postfix:
//                     kDebug() << "Postfix";
                    break;
            }
            break;
        case InheritanceDepth:
//             kDebug() << "InheritanceDepth";
            return 0;
        case ArgumentHintDepth:
//             kDebug() << "ArgumentHintDepth";
            return 0;
        case CompletionRole:
//             kDebug() << "CompletionRole";
            return QVariant(GlobalScope | LocalScope);
        case ItemSelected:
//             kDebug() << "ItemSelected";
//             return QString("Tooltip text");
        default:
            break;
    }
    return QVariant();
}

void IncludeHelperPluginCompletionModel::executeCompletionItem2(
    KTextEditor::Document* document
  , const KTextEditor::Range& word
  , const QModelIndex& index
  ) const
{
    // Call default handler
    QString p = index.row() < m_dir_completions.size()
      ? m_dir_completions[index.row()]
      : m_file_completions[index.row() - m_dir_completions.size()]
      ;
    kDebug() << "exec: " << p;
    if (index.row() >= m_dir_completions.size() && m_closer == '>')
        p += m_closer;
    document->replaceText(word, p);
}

KTextEditor::Range IncludeHelperPluginCompletionModel::completionRange(
    KTextEditor::View* view
  , const KTextEditor::Cursor& position
  )
{
    kDebug() << "cursor: " << position;
//     if (m_should_complete)
//     {
//         QString text = view->document()->line(position.line());
//         kDebug() << "t2p: " << text;
//         KTextEditor::Range result = parseIncludeDirective(text, false);
//         result.setBothLines(position.line());
//         kDebug() << "result: " << result;
//         return result;
//     }
    return KTextEditor::CodeCompletionModelControllerInterface3::completionRange(view, position);
}
//END IncludeHelperPluginCompletionModel
}                                                           // namespace kate
