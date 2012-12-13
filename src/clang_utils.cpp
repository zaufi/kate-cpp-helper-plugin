/**
 * \file
 *
 * \brief Some helpers to deal w/ clang API
 *
 * \date Mon Nov 19 04:31:41 MSK 2012 -- Initial design
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
#include <src/clang_utils.h>

// Standard includes
#include <map>

#define MAP_ENTRY(x) {x, #x}

namespace kate { namespace {

const std::map<CXCompletionChunkKind, QString> CHUNK_KIND_TO_STRING = {
    MAP_ENTRY(CXCompletionChunk_Optional)
  , MAP_ENTRY(CXCompletionChunk_TypedText)
  , MAP_ENTRY(CXCompletionChunk_Text)
  , MAP_ENTRY(CXCompletionChunk_Placeholder)
  , MAP_ENTRY(CXCompletionChunk_Informative)
  , MAP_ENTRY(CXCompletionChunk_CurrentParameter)
  , MAP_ENTRY(CXCompletionChunk_LeftParen)
  , MAP_ENTRY(CXCompletionChunk_RightParen)
  , MAP_ENTRY(CXCompletionChunk_LeftBracket)
  , MAP_ENTRY(CXCompletionChunk_RightBracket)
  , MAP_ENTRY(CXCompletionChunk_LeftBrace)
  , MAP_ENTRY(CXCompletionChunk_RightBrace)
  , MAP_ENTRY(CXCompletionChunk_LeftAngle)
  , MAP_ENTRY(CXCompletionChunk_RightAngle)
  , MAP_ENTRY(CXCompletionChunk_Comma)
  , MAP_ENTRY(CXCompletionChunk_ResultType)
  , MAP_ENTRY(CXCompletionChunk_Colon)
  , MAP_ENTRY(CXCompletionChunk_SemiColon)
  , MAP_ENTRY(CXCompletionChunk_Equal)
  , MAP_ENTRY(CXCompletionChunk_HorizontalSpace)
  , MAP_ENTRY(CXCompletionChunk_VerticalSpace)
};
}                                                           // anonymous namespace

/// Get a human readable string of \c CXCompletionChunkKind
QString toString(const CXCompletionChunkKind v) try
{
#if 0
    /// \todo This expected to work but it doesn't due undefined function ;(
    DCXString str = clang_getCompletionChunkKindSpelling(v);
    return QString(clang_getCString(str));
#endif
    return CHUNK_KIND_TO_STRING.at(v);
}
catch (const std::exception&)
{
    return QString::number(v);
}
}                                                           // namespace kate
#undef MAP_ENTRY
// kate: hl C++11/Qt4;
