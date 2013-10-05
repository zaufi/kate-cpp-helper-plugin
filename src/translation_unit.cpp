/**
 * \file
 *
 * \brief Class \c kate::TranslationUnit (implementation)
 *
 * \date Thu Nov 22 18:07:27 MSK 2012 -- Initial design
 *
 * \todo Add \c i18n() to (some) strings?
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
#include <src/translation_unit.h>
#include <src/clang_utils.h>
#include <src/sanitize_snippet.h>

// Standard includes
#include <cassert>

#if defined(CINDEX_VERSION_MAJOR) && defined(CINDEX_VERSION_MINOR)
# if CINDEX_VERSION_MAJOR == 0 && CINDEX_VERSION_MINOR == 6
// libclang got version macros since clang 3.2
#   define CLANG_VERSION 30200
# else
// Some future version...
#   define CLANG_VERSION 39999
# endif
#else
// clang piror 3.2
# define CLANG_VERSION 30000
#endif

namespace kate { namespace {
/// Show some debug info about completion string from Clang
void debugShowCompletionResult(
    const unsigned i
  , const unsigned priority
  , const CXCompletionString str
  , const CXCursorKind cursor_kind
  )
{
    kDebug(DEBUG_AREA) << ">>> Completion " << i
        << ", priority " << priority
        << ", kind " << toString(cursor_kind).toUtf8().constData();

    // Show info about parent context
    CXCursorKind parent_ck;
    const DCXString parent = {clang_getCompletionParent(str, &parent_ck)};
    const auto parent_str = clang_getCString(parent);
    kDebug(DEBUG_AREA) << "  parent: " << (parent_str ? parent_str : "<none>")
      << ',' << toString(parent_ck).toUtf8().constData();

    // Show individual chunks
    for (unsigned ci = 0, cn = clang_getNumCompletionChunks(str); ci < cn; ++ci)
    {
        auto kind = clang_getCompletionChunkKind(str, ci);
        if (kind == CXCompletionChunk_Optional)
        {
            kDebug(DEBUG_AREA) << "  chunk [" << i << ':' << ci << "]: " << toString(kind).toUtf8().constData();
            auto ostr = clang_getCompletionChunkCompletionString(str, ci);
            for (unsigned oci = 0, ocn = clang_getNumCompletionChunks(ostr); oci < ocn; ++oci)
            {
                auto okind = clang_getCompletionChunkKind(ostr, oci);
                DCXString otext_str = {clang_getCompletionChunkText(ostr, oci)};
                kDebug(DEBUG_AREA) << "  chunk [" << i << ':' << ci << ':' << oci << "]: "
                  << toString(okind).toUtf8().constData()
                  << ", text=" << QString(clang_getCString(otext_str));
                  ;
            }
        }
        else
        {
            DCXString text_str = {clang_getCompletionChunkText(str, ci)};
            kDebug(DEBUG_AREA) << "  chunk [" << i << ':' << ci << "]: " << toString(kind).toUtf8().constData()
              << ", text=" << QString(clang_getCString(text_str));
        }
    }
#if CLANG_VERSION >= 30200
    DCXString comment_str = {clang_getCompletionBriefComment(str)};
    kDebug(DEBUG_AREA) << "  comment:" << QString(clang_getCString(comment_str));
#endif                                                      // CLANG_VERSION >= 30200

    // Show annotations
    for (unsigned ai = 0u, an = clang_getCompletionNumAnnotations(str); ai < an; ++ai)
    {
        auto ann_str = clang_getCompletionAnnotation(str, an);
        kDebug(DEBUG_AREA) << "  ann. text[" << ai << "]: " << QString(clang_getCString(ann_str));
    }

    // Show availability
    const char* av = "unknown";
    switch (clang_getCompletionAvailability(str))
    {
        case CXAvailability_Available:
            av = "available";
            break;
        case CXAvailability_Deprecated:
            av = "deprecated";
            break;
        case CXAvailability_NotAvailable:
            av = "not available";
            break;
        case CXAvailability_NotAccessible:
            av = "not accessible";
            break;
    }
    kDebug(DEBUG_AREA) << "  availability: " <<  av;
    kDebug(DEBUG_AREA) << ">>> -----------------------------------";
}

const QString GLOBAL_NS_GROUP_STR = {"Global"};
const QString PREPROCESSOR_GROUP_STR = {"Preprocessor Macro"};

const QString STRUCT_NS_STR = {"struct"};
const QString ENUM_NS_STR = {"enum"};
const QString UNION_NS_STR = {"union"};
const QString CLASS_NS_STR = {"class"};
const QString TYPEDEF_NS_STR = {"typedef"};
const QString NAMESPACE_NS_STR = {"namespace"};
}                                                           // anonymous namespace

/**
 * This constructor can be used to load previously translated and saved unit:
 * the typical way is to load a PCH files
 */
TranslationUnit::TranslationUnit(
    CXIndex index
  , const KUrl& filename_url
  )
  : m_filename(filename_url.toLocalFile().toUtf8())
  , m_unit(clang_createTranslationUnit(index, m_filename.constData()))
{
    if (!m_unit)
        throw Exception::LoadFailure("Fail to load a preparsed file");
}

TranslationUnit::TranslationUnit(
    CXIndex index
  , const KUrl& filename_url
  , const QStringList& options
  )
  : m_filename(filename_url.toLocalFile().toUtf8())
{
    kDebug(DEBUG_AREA) << "Creating a translation unit: " << filename_url.toLocalFile();
    kDebug(DEBUG_AREA) << "w/ the following compiler options:" << options;
    // Transform options compatible to clang API
    // NOTE Too fraking much actions for that simple task...
    // Definitely Qt doesn't suitable for that sort of tasks...
    std::vector<QByteArray> utf8_options(options.size());
    std::vector<const char*> clang_options(options.size(), nullptr);
    {
        int opt_idx = 0;
        for (const auto& o : options)
        {
            utf8_options[opt_idx] = o.toUtf8();
            clang_options[opt_idx] = utf8_options[opt_idx].constData();
            opt_idx++;
        }
    }
    // Ok, ready to go
    m_unit = clang_createTranslationUnitFromSourceFile(
        index
      , m_filename.constData()
      , clang_options.size()
      , clang_options.data()
      , 0
      , 0
      );
    if (!m_unit)
        throw Exception::ParseFailure("Failure to parse C++ code");
}

TranslationUnit::TranslationUnit(
    CXIndex index
  , const KUrl& filename_url
  , const QStringList& options
  , const unsigned parse_options
  , const unsaved_files_list_type& unsaved_files
  )
  : m_filename(filename_url.toLocalFile().toUtf8())
{
    kDebug(DEBUG_AREA) << "Parsing a translation unit: " << filename_url.toLocalFile();
    kDebug(DEBUG_AREA) << "w/ the following compiler options:" << options;

    // Transform options compatible to clang API
    // NOTE Too fraking much actions for that simple task...
    // Definitely Qt doesn't suitable for that sort of tasks...
    std::vector<QByteArray> utf8_options(options.size());
    std::vector<const char*> clang_options(options.size(), nullptr);
    {
        int opt_idx = 0;
        for (const auto& o : options)
        {
            utf8_options[opt_idx] = o.toUtf8();
            clang_options[opt_idx] = utf8_options[opt_idx].constData();
            opt_idx++;
        }
    }

    // Setup internal structures w/ unsaved files
    updateUnsavedFiles(unsaved_files);

    // Ok, ready to parse
    m_unit = clang_parseTranslationUnit(
        index
      , m_filename.constData()
      , clang_options.data()
      , clang_options.size()
      , m_unsaved_files.data()
      , m_unsaved_files.size()
      , parse_options
      );
    if (!m_unit)
        throw Exception::ParseFailure("Failure to parse C++ code");
    updateDiagnostic();
}

TranslationUnit::TranslationUnit(TranslationUnit&& other) noexcept
  : m_unsaved_files_utf8(std::move(other.m_unsaved_files_utf8))
  , m_unsaved_files(std::move(other.m_unsaved_files))
  , m_unit(other.m_unit)
{
    m_filename.swap(other.m_filename);
    other.m_unit = nullptr;
}

TranslationUnit& TranslationUnit::operator=(TranslationUnit&& other) noexcept
{
    m_unsaved_files_utf8 = std::move(other.m_unsaved_files_utf8);
    m_unsaved_files = std::move(other.m_unsaved_files);
    m_filename.swap(other.m_filename);
    m_unit = other.m_unit;
    other.m_unit = nullptr;
    return *this;
}

void TranslationUnit::updateUnsavedFiles(const unsaved_files_list_type& unsaved_files)
{
    // Resize internal vectors according expected items count
    m_unsaved_files_utf8.resize(unsaved_files.size());
    m_unsaved_files.resize(unsaved_files.size());
    // Transform unsaved files compatible to clang API
    int uf_idx = 0;
    for (const auto& p : unsaved_files)
    {
        m_unsaved_files_utf8[uf_idx] = {p.first.toUtf8(), p.second.toUtf8()};
        m_unsaved_files[uf_idx].Filename = m_unsaved_files_utf8[uf_idx].first.constData();
        m_unsaved_files[uf_idx].Contents = m_unsaved_files_utf8[uf_idx].second.constData();
        /// \note Fraking \c QByteArray has \c int as return type of \c size()! IDIOTS!
        m_unsaved_files[uf_idx].Length = unsigned(m_unsaved_files_utf8[uf_idx].second.size());
        uf_idx++;
    }
}

QList<ClangCodeCompletionItem> TranslationUnit::completeAt(
    const int line
  , const int column
  , const unsigned completion_flags
  , const PluginConfiguration::sanitize_rules_list_type& sanitize_rules
  )
{
    QList<ClangCodeCompletionItem> completions;

    DCXCodeCompleteResults res = {
        clang_codeCompleteAt(
            m_unit
          , m_filename.constData()
          , unsigned(line)
          , unsigned(column)
          , m_unsaved_files.data()
          , m_unsaved_files.size()
          , completion_flags
          )
      };
    if (!res)
    {
        throw Exception::CompletionFailure("Unable to perform code completion");
    }

#if 0
    clang_sortCodeCompletionResults(res->Results, res->NumResults);
#endif

    // Collect some diagnostic SPAM
    for (unsigned i = 0; i < clang_codeCompleteGetNumDiagnostics(res); ++i)
    {
        DCXDiagnostic diag = {clang_codeCompleteGetDiagnostic(res, i)};
        appendDiagnostic(diag);
    }

    completions.reserve(res->NumResults);                   // Peallocate enough space for completion results

    // Lets look what we've got...
    for (unsigned i = 0; i < res->NumResults; ++i)
    {
        const auto str = res->Results[i].CompletionString;
        const auto priority = clang_getCompletionPriority(str);
        const auto cursor_kind = res->Results[i].CursorKind;
        debugShowCompletionResult(i, priority, str, cursor_kind);

        // Skip unusable completions
        // 0) check availability
        const auto availability = clang_getCompletionAvailability(str);
        if (availability != CXAvailability_Available && availability != CXAvailability_Deprecated)
        {
            kDebug(DEBUG_AREA) << "!! Skip result" << i << "as not available";
            continue;
        }
        // 1) check usefulness
        /// \todo Make it configurable
        if (cursor_kind == CXCursor_NotImplemented)
            continue;

        // Collect all completion chunks and from a format string
        QString text_before;
        QString typed_text;
        QString text_after;
        QStringList placeholders;
        int optional_placeholers_start_position = -1;
        // A lambda to append given text to different parts
        // of future completion string, depending on already processed text
        auto appender = [&](const QString& text)
        {
            if (typed_text.isEmpty())
                text_before += text;
            else
                text_after += text;
        };
        bool skip_this_item = false;
        for (
            unsigned j = 0
          , chunks = clang_getNumCompletionChunks(str)
          ; j < chunks && !skip_this_item
          ; ++j
          )
        {
            auto kind = clang_getCompletionChunkKind(str, j);
            DCXString text_str = {clang_getCompletionChunkText(str, j)};
            QString text = clang_getCString(text_str);
            switch (kind)
            {
                // Text that a user would be expected to type to get this code-completion result
                case CXCompletionChunk_TypedText:
                // Text that should be inserted as part of a code-completion result
                case CXCompletionChunk_Text:
                {
                    auto p = sanitize(text, sanitize_rules);// Pipe given piece of text through sanitizer
                    if (p.first)
                        typed_text += p.second;
                    else                                    // Go for next completion item
                        skip_this_item = true;
                    break;
                }
                // Placeholder text that should be replaced by the user
                case CXCompletionChunk_Placeholder:
                {
                    auto p = sanitize(text, sanitize_rules);// Pipe given piece of text through sanitizer
                    if (p.first)
                    {
                        appender("%" + QString::number(placeholders.size() + 1) + "%");
                        placeholders.push_back(p.second);
                    }
                    else                                    // Go for next completion item
                        skip_this_item = true;
                    break;
                }
                // A code-completion string that describes "optional" text that
                // could be a part of the template (but is not required)
                case CXCompletionChunk_Optional:
                {
                    auto ostr = clang_getCompletionChunkCompletionString(str, j);
                    for (
                        unsigned oci = 0
                      , ocn = clang_getNumCompletionChunks(ostr)
                      ; oci < ocn
                      ; ++oci
                      )
                    {
                        DCXString otext_str = {clang_getCompletionChunkText(ostr, oci)};
                        QString otext{clang_getCString(otext_str)};
                        // Pipe given piece of text through sanitizer
                        auto p = sanitize(otext, sanitize_rules);
                        if (p.first)
                        {
                            auto okind = clang_getCompletionChunkKind(ostr, oci);
                            if (okind == CXCompletionChunk_Placeholder)
                            {
                                appender("%" + QString::number(placeholders.size() + 1) + "%");
                                placeholders.push_back(p.second);
                                optional_placeholers_start_position = placeholders.size();
                            }
                            else appender(p.second);
                        }
                        else
                        {
                            skip_this_item = true;
                            break;
                        }
                    }
                    break;
                }
                case CXCompletionChunk_ResultType:
                case CXCompletionChunk_LeftParen:
                case CXCompletionChunk_RightParen:
                case CXCompletionChunk_LeftBracket:
                case CXCompletionChunk_RightBracket:
                case CXCompletionChunk_LeftBrace:
                case CXCompletionChunk_RightBrace:
                case CXCompletionChunk_LeftAngle:
                case CXCompletionChunk_RightAngle:
                case CXCompletionChunk_Comma:
                case CXCompletionChunk_Colon:
                case CXCompletionChunk_SemiColon:
                case CXCompletionChunk_Equal:
                case CXCompletionChunk_CurrentParameter:
                case CXCompletionChunk_HorizontalSpace:
                /// \todo Kate can't handle \c '\n' well in completions list
                case CXCompletionChunk_VerticalSpace:
                {
                    auto p = sanitize(text, sanitize_rules);// Pipe given piece of text through sanitizer
                    if (p.first)
                        appender(p.second);
                    else                                    // Go for next completion item
                        skip_this_item = true;
                    break;
                }
                // Informative text that should be displayed but never inserted
                // as part of the template
                case CXCompletionChunk_Informative:
                    // Informative text before CXCompletionChunk_TypedText usually
                    // just a method scope (i.e. long name of an owner class)
                    // and it's useless for completer cuz it can group items
                    // by parent already...
                    if (!typed_text.isEmpty())
                    {
                        // Pipe given piece of text through sanitizer
                        auto p = sanitize(text, sanitize_rules);
                        if (p.first)
                            appender(p.second);
                        else                                // Go for next completion item
                            skip_this_item = true;
                    }
                    break;
                default:
                    break;
            }
        }
        // Does it pass the completion items sanitizer?
        if (skip_this_item) continue;                       // No! Skip it!

        assert("Priority expected to be less than 100" && priority < 101u);
        //
        completions.push_back({
            makeParentText(str, cursor_kind)
          , text_before
          , typed_text
          , text_after
          , placeholders
          , optional_placeholers_start_position
          , priority
          , cursor_kind
          , availability == CXAvailability_Deprecated
          });
    }

    return completions;
}

/**
 * Get human readable text representation of a parent context of the given completion string
 */
QString TranslationUnit::makeParentText(CXCompletionString str, const CXCursorKind cur_kind)
{
    CXCursorKind parent_ck;
    const DCXString parent_clstr = {clang_getCompletionParent(str, &parent_ck)};
    const auto parent_cstr = clang_getCString(parent_clstr);
    if (parent_cstr)
    {
        const auto parent_str = QString(parent_cstr).trimmed();
        if (parent_str.isEmpty())
            return GLOBAL_NS_GROUP_STR;
        QString prefix;
        switch (parent_ck)
        {
            case CXCursor_StructDecl:
                prefix = STRUCT_NS_STR;
                break;
            case CXCursor_UnionDecl:
                prefix = UNION_NS_STR;
                break;
            case CXCursor_ClassDecl:
                prefix = CLASS_NS_STR;
                break;
            case CXCursor_EnumDecl:
                prefix = ENUM_NS_STR;
                break;
            case CXCursor_Namespace:
                prefix = NAMESPACE_NS_STR;
                break;
            /// \todo More types?
            default:
                break;
        }
        if (!prefix.isEmpty())
            prefix += ' ';
        return prefix + parent_str;
    }
    else if (cur_kind == CXCursor_MacroDefinition)
        return PREPROCESSOR_GROUP_STR;
    return GLOBAL_NS_GROUP_STR;
}

void TranslationUnit::storeTo(const KUrl& filename)
{
    QByteArray pch_filename = filename.toLocalFile().toUtf8();
    auto result = clang_saveTranslationUnit(
        m_unit
      , pch_filename.constData()
      , CXSaveTranslationUnit_None /*clang_defaultSaveOptions(m_unit)*/
      );
    kDebug(DEBUG_AREA) << "result=" << result;
    if (result != CXSaveError_None)
    {
        if (result == CXSaveError_TranslationErrors)
            updateDiagnostic();
        throw Exception::SaveFailure("Failure on save translation unit into a file");
    }
}

void TranslationUnit::reparse()
{
    auto result = clang_reparseTranslationUnit(
        m_unit
      , m_unsaved_files.size()
      , m_unsaved_files.data()
      , CXReparse_None /*clang_defaultReparseOptions(m_unit)*/
      );
    if (result)
    {
        throw Exception::ReparseFailure("It seems preparsed file is invalid");
    }
}

/**
 * \attention \c clang_formatDiagnostic have a nasty BUG since clang 3.3!
 * It fails (<em>pure virtual function call</em>) on messages w/o location attached
 * (like notices). DO NOT USE IT! EVER!
 */
void TranslationUnit::appendDiagnostic(const CXDiagnostic& diag)
{
    // Should we ignore this item?
    const auto severity = clang_getDiagnosticSeverity(diag);
    if (severity == CXDiagnostic_Ignored)
        return;
    kDebug(DEBUG_AREA) << "TU diagnostic severity level: " << severity;

    // Get record type
    DiagnosticMessagesModel::Record::type type;
    switch (severity)
    {
        case CXDiagnostic_Note:
            type = DiagnosticMessagesModel::Record::type::info;
            break;
        case CXDiagnostic_Warning:
            type = DiagnosticMessagesModel::Record::type::warning;
            break;
        case CXDiagnostic_Error:
        case CXDiagnostic_Fatal:
            type = DiagnosticMessagesModel::Record::type::error;
            break;
        default:
            assert(!"Unexpected severity level! Code review required!");
    }

    // Get location
    location loc;
    /// \attention \c Notes have no location attached!?
    if (severity != CXDiagnostic_Note)
    {
        try
        {
            loc = {clang_getDiagnosticLocation(diag)};
        }
        catch (std::exception& e)
        {
            kDebug(DEBUG_AREA) << "TU diag.fmt: Can't get diagnostic location";
        }
    }

    // Get diagnostic text and form a new diagnostic record
    m_last_diagnostic_messages.emplace_back(
        std::move(loc)
      , toString(clang_getDiagnosticSpelling(diag))
      , type
      );
}

void TranslationUnit::updateDiagnostic()
{
    for (unsigned i = 0, last = clang_getNumDiagnostics(m_unit); i < last; ++i)
    {
        DCXDiagnostic diag = {clang_getDiagnostic(m_unit, i)};
        appendDiagnostic(diag);
    }
}

unsigned TranslationUnit::defaultPCHParseOptions()
{
    return CXTranslationUnit_Incomplete
      | CXTranslationUnit_PrecompiledPreamble
      | CXTranslationUnit_SkipFunctionBodies
      | CXTranslationUnit_CacheCompletionResults
#if 0
      | CXTranslationUnit_DetailedPreprocessingRecord
#endif
#if CLANG_VERSION >= 30200
      | CXTranslationUnit_ForSerialization
#endif                                                      // CLANG_VERSION >= 30200
      ;
}

unsigned TranslationUnit::defaultEditingParseOptions()
{
    return clang_defaultEditingTranslationUnitOptions()
      | CXTranslationUnit_Incomplete
      | CXTranslationUnit_PrecompiledPreamble
      | CXTranslationUnit_CacheCompletionResults
#if CLANG_VERSION >= 30200
      | CXTranslationUnit_IncludeBriefCommentsInCodeCompletion
#endif                                                      // CLANG_VERSION >= 30200
      ;
}

unsigned TranslationUnit::defaultExplorerParseOptions()
{
    return CXTranslationUnit_Incomplete | CXTranslationUnit_SkipFunctionBodies;
}

/**
 * Transform comman line options into a form compatible for clang API
 *
 * \note Too fraking much actions for that simple task...
 * Definitely Qt doesn't suitable for that sort of tasks...
 */
void TranslationUnit::transform_command_line_args(
    QStringList& input
  , std::vector<QByteArray>& args
  , std::vector<const char*>& pointers
  )
{
    args.resize(input.size());
    pointers.resize(input.size(), nullptr);
    int opt_idx = 0;
    for (const auto& o : input)
    {
        args[opt_idx] = o.toUtf8();
        pointers[opt_idx] = args[opt_idx].constData();
        opt_idx++;
    }
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
