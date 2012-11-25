/**
 * \file
 *
 * \brief Class \c kate::TranslationUnit (interface)
 *
 * \date Thu Nov 22 18:07:27 MSK 2012 -- Initial design
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

#ifndef __SRC__TRANSLATION_UNIT_H__
# define __SRC__TRANSLATION_UNIT_H__

// Project specific includes
# include <src/clang_code_completion_item.h>

// Standard includes
# include <KDE/KUrl>
# include <QtCore/QPair>
# include <QtCore/QStringList>
# include <QtCore/QVector>
# include <stdexcept>
# include <vector>
# include <utility>

namespace kate {

/**
 * \brief [Type brief class description here]
 *
 * [More detailed description here]
 *
 */
class TranslationUnit
{
public:
    enum struct ParseOptions : unsigned
    {
        None = CXTranslationUnit_None
      , PCHOptions = CXTranslationUnit_PrecompiledPreamble
          | CXTranslationUnit_Incomplete
          //| CXTranslationUnit_SkipFunctionBodies
      , CompletionOptions = CXTranslationUnit_Incomplete /*| CXTranslationUnit_CacheCompletionResults*/
    };
    struct Exception : public std::runtime_error
    {
        struct CompletionFailure;
        struct LoadFailure;
        struct ParseFailure;
        struct ReparseFailure;
        struct SaveFailure;

        explicit Exception(const std::string&);
    };
    /// Make a translation unit from a previously serialized file (PCH)
    TranslationUnit(CXIndex, const KUrl&);
    /// Make a translation unit from a given source file
    TranslationUnit(
        CXIndex
      , const KUrl&
      , const QStringList&
      , const ParseOptions
      , const QVector<QPair<QString, QString>>& = QVector<QPair<QString, QString>>()
      );
    /// Destructor
    virtual ~TranslationUnit()
    {
        clang_disposeTranslationUnit(m_unit);
    }

    void updateUnsavedFiles(const QVector<QPair<QString, QString>>&);
    QList<ClangCodeCompletionItem> completeAt(const int, const int);
    void storeTo(const KUrl&);
    void reparse();

private:
    void showDiagnostic();

    std::vector<std::pair<QByteArray, QByteArray>> m_unsaved_files_utf8;
    std::vector<CXUnsavedFile> m_unsaved_files;
    QByteArray m_filename;
    CXTranslationUnit m_unit;
};

struct TranslationUnit::Exception::CompletionFailure : public TranslationUnit::Exception
{
    explicit CompletionFailure(const std::string& str) : Exception(str) {}
};
struct TranslationUnit::Exception::LoadFailure : public TranslationUnit::Exception
{
    explicit LoadFailure(const std::string& str) : Exception(str) {}
};
struct TranslationUnit::Exception::ParseFailure : public TranslationUnit::Exception
{
    explicit ParseFailure(const std::string& str) : Exception(str) {}
};
struct TranslationUnit::Exception::ReparseFailure : public TranslationUnit::Exception
{
    explicit ReparseFailure(const std::string& str) : Exception(str) {}
};
struct TranslationUnit::Exception::SaveFailure : public TranslationUnit::Exception
{
    explicit SaveFailure(const std::string& str) : Exception(str) {}
};

inline TranslationUnit::Exception::Exception(const std::string& str) : std::runtime_error(str) {}

}                                                           // namespace kate
#endif                                                      // __SRC__TRANSLATION_UNIT_H__
// kate: hl C++11/Qt4;
