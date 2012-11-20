/**
 * \file
 *
 * \brief Class \c kate::ClangCodeCompletionModel (implementation)
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

// Project specific includes
#include <src/clang_code_completion_model.h>
#include <src/clang_utils.h>
#include <src/include_helper_plugin.h>

// Standard includes
#include <KLocalizedString>                                 /// \todo Where is \c i18n() defiend?
#include <KTextEditor/Document>
#include <KTextEditor/View>

namespace kate {

ClangCodeCompletionModel::ClangCodeCompletionModel(
    QObject* parent
  , IncludeHelperPlugin* plugin
  )
  : KTextEditor::CodeCompletionModel2(parent)
  , m_plugin(plugin)
{
    
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

    KTextEditor::Document* doc = view->document();
    KUrl url = doc->url();
    if (!url.isValid() || url.isEmpty())
    {
        /// \todo Turn into a popup
        kWarning() << "U have to have a document on a disk before use code completion";
        return;
    }

    kDebug() << "It seems user has invoked comletion at " << range;
    /// \todo Make parameters to \c clang_createIndex() configurable
    DCXIndex index = clang_createIndex(1, 0);
        if (!index)
    {
        /// \todo Turn into a popup
        kWarning() << "Fail to make an index";
        return;
    }

    // Remove everything collected before
    m_completions.clear();

    // Form command line parameters
    // 1) collect configured system and session dirs and make -I option series
    QList<QByteArray> options;
    // reserve space for at least known options count
    options.reserve(m_plugin->systemDirs().size() + m_plugin->sessionDirs().size());
    Q_FOREACH(const QString& dir, m_plugin->systemDirs())
        options.push_back(QString("-I" + dir).toUtf8());
    Q_FOREACH(const QString& dir, m_plugin->sessionDirs())
        options.push_back(QString("-I" + dir).toUtf8());
    // 2) split configured aux options and append to collected
    Q_FOREACH(const QString& o, m_plugin->clangParams().split(' ', QString::SkipEmptyParts))
        options.push_back(o.toUtf8());
    // 3) append PCH options (TBD)
    // 4) form a plain array of arguments finally
    QVector<const char*> clang_options;
    clang_options.reserve(options.size());
    Q_FOREACH(const QByteArray& o, options)
        clang_options.push_back(o.constData());
    kDebug() << "Collected options: " << clang_options;
    // Form unsaved files list
    typedef QPair<QByteArray, QByteArray> pair_of_arrays;
    QVector<pair_of_arrays> unsaved_files_storage;
    // 1) append this document to the list of unsaved files
    QByteArray this_filename = url.toLocalFile().toUtf8();
    unsaved_files_storage.push_back(
        qMakePair(this_filename, doc->text().toUtf8())
      );
    /// \todo Collect all unsaved files
    // Transform collected files to clang's structure
    QVector<CXUnsavedFile> unsaved_files;
    unsaved_files.reserve(unsaved_files_storage.size());
    Q_FOREACH(const pair_of_arrays& p, unsaved_files_storage)
    {
        /// \note Fracking \c QByteArray has \c int as return type of \c size()! IDIOTS!
        CXUnsavedFile f = {p.first.constData(), p.second.constData(), unsigned(p.second.size())};
        unsaved_files.push_back(f);
    }
    //
    DCXTranslationUnit unit = clang_createTranslationUnitFromSourceFile(
        index
      , this_filename.constData()
      , clang_options.size()
      , clang_options.constData()
      , unsaved_files.size()
      , unsaved_files.data()
      );
    if (!unit)
    {
        kWarning() << "Fail to make an instance of translation unit";
        return;
    }

    // Try to make a comletion
    DCXCodeCompleteResults res = clang_codeCompleteAt(
        unit
      , this_filename.constData()
      , unsigned(range.start().line() + 1)                  // NOTE Kate count lines starting from 0
      , unsigned(range.start().column() + 1)                // NOTE Kate count columns starting from 0
      , unsaved_files.data()
      , unsaved_files.size()
      , CXCodeComplete_IncludeCodePatterns                  /// \todo Make options configurabe
      );
    if (!res)
    {
        kWarning() << "Could not complete";
        return;
    }
    clang_sortCodeCompletionResults(res->Results, res->NumResults);

    // Show some SPAM
    for (unsigned i = 0; i < clang_codeCompleteGetNumDiagnostics(res); ++i)
    {
        DCXDiagnostic diag = clang_codeCompleteGetDiagnostic(res, i);
        DCXString s = clang_getDiagnosticSpelling(diag);
        kWarning() << clang_getCString(s);
    }

    for (unsigned i = 0; i < res->NumResults; ++i)
    {
        const auto str = res->Results[i].CompletionString;
        const auto priority = clang_getCompletionPriority(str);
        kDebug() << ">>> Completion " << i
          << ", priority " << priority
          << ", kind " << toString(res->Results[i].CursorKind);
        // Skip unusable completions
        if (res->Results[i].CursorKind == CXCursor_NotImplemented)
            continue;
        // Collect all completion chunks and from a format string
        QString text_before;
        QString typed_text;
        QString text_after;
        QStringList placeholders;
        auto appender = [&](const QString& text)
        {
            if (typed_text.isEmpty())
                text_before += text;
            else
                text_after += text;
        };
        for (unsigned j = 0; j < clang_getNumCompletionChunks(str); ++j)
        {
            auto kind = clang_getCompletionChunkKind(str, j);
            kDebug() << ">>> Completion " << i << ", chunk " << j << ", kind: " << toString(kind);
            DCXString text_str = clang_getCompletionChunkText(str, j);
            QString text = clang_getCString(text_str);
            kDebug() << ">>> Completion text: " << text;
            switch (kind)
            {
                case CXCompletionChunk_TypedText:
                    assert(!"Sanity check" && typed_text.isEmpty());
                    typed_text = text;
                    break;
                case CXCompletionChunk_ResultType:
                    appender(text);
                    break;
                case CXCompletionChunk_Placeholder:
                    appender("%" + QString::number(placeholders.size() + 1) + "%");
                    placeholders.push_back(text);
                    break;
                default:
                    appender(text);
                    break;
            }
        }
        m_completions.insert({priority, {text_before, typed_text, text_after, placeholders}});
        kDebug() << "Completion item: fmt="
          << (text_before + " " + typed_text + text_after)
          << ", placeholders: " << placeholders
          ;
        kDebug() << ">>> -----------------------------------";
    }

}

namespace {

}                                                           // anonymous namespace

QVariant ClangCodeCompletionModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    // grouping of completions
    if (!index.parent().isValid())
    {
        if (role == Qt::DisplayRole)
            return i18n("Code Completions");
        if (role == KTextEditor::CodeCompletionModel::GroupRole)
            return Qt::DisplayRole;
        return QVariant();
    }
    // completions
    if (!index.isValid() || index.row() < 0 || std::size_t(index.row()) >= m_completions.size())
        return QVariant();

    auto it = m_completions.begin();
    std::advance(it, index.row());
    return it->second.data(index, role);
}

int ClangCodeCompletionModel::columnCount(const QModelIndex&) const
{
    return 1;
}

int ClangCodeCompletionModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid() && !m_completions.empty())
        return 1;                                           // one toplevel node (group header)
    if(parent.parent().isValid())
        return 0;                                           // we don't have sub children

    return m_completions.size();                            // only the children
}

QModelIndex ClangCodeCompletionModel::parent(const QModelIndex& index) const
{
    // make a ref to root node from level 1 nodes,
    // otherwise return an invalid node.
    return index.internalId() ? createIndex(0, 0, 0) : QModelIndex();
}

QModelIndex ClangCodeCompletionModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!parent.isValid())
    {
        if (row == 0)
            return createIndex(row, column, 0);             // header  index
        return QModelIndex();
    }                                                       // we only have header and children, no subheaders
    if (parent.parent().isValid())
        return QModelIndex();

    if (row < 0 || std::size_t(row) >= m_completions.size() || column < 0 || column >= ColumnCount )
        return QModelIndex();

    return createIndex(row, column, 1);                     // normal item index
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
