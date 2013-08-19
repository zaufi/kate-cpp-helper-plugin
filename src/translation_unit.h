/**
 * \file
 *
 * \brief Class \c kate::TranslationUnit (interface)
 *
 * \date Thu Nov 22 18:07:27 MSK 2012 -- Initial design
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

#ifndef __SRC__TRANSLATION_UNIT_H__
# define __SRC__TRANSLATION_UNIT_H__

// Project specific includes
# include <src/clang_code_completion_item.h>
# include <src/diagnostic_messages_model.h>
# include <src/plugin_configuration.h>

// Standard includes
# include <KUrl>
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
    struct Exception : public std::runtime_error
    {
        struct CompletionFailure;
        struct LoadFailure;
        struct ParseFailure;
        struct ReparseFailure;
        struct SaveFailure;

        explicit Exception(const std::string&);
    };
    typedef std::vector<DiagnosticMessagesModel::Record> records_list_type;
    typedef QVector<QPair<QString, QString>> unsaved_files_list_type;
    /// Make a translation unit from a previously serialized file (PCH)
    TranslationUnit(CXIndex, const KUrl&);
    /// Make a translation unit from a given source file
    TranslationUnit(
        CXIndex
      , const KUrl&
      , const QStringList&
      );
    /// Make a translation unit from a given source file
    TranslationUnit(
        CXIndex
      , const KUrl&
      , const QStringList&
      , unsigned
      , const unsaved_files_list_type& = unsaved_files_list_type()
      );
    /// Move ctor
    TranslationUnit(TranslationUnit&&) noexcept;
    /// Move-assign operator
    TranslationUnit& operator=(TranslationUnit&&) noexcept;
    /// Delete copy ctor
    TranslationUnit(const TranslationUnit&) = delete;
    /// Delete copy-assign operator
    TranslationUnit& operator=(const TranslationUnit&) = delete;
    /// Destructor
    virtual ~TranslationUnit()
    {
        if (m_unit)
            clang_disposeTranslationUnit(m_unit);
    }

    /// Allow implicit conversion to \c CXTranslationUnit, so clang index API
    /// cound be fed w/ instances of this class
    operator CXTranslationUnit()
    {
        return m_unit;
    }
    operator CXTranslationUnit() const
    {
        return m_unit;
    }

    void updateUnsavedFiles(const unsaved_files_list_type&);
    QList<ClangCodeCompletionItem> completeAt(
        int
      , int
      , unsigned
      , const PluginConfiguration::sanitize_rules_list_type&
      );
    void storeTo(const KUrl&);
    void reparse();

    /// Obtain diagnostic messages after last operation
    /// \note Leave internal container empty
    records_list_type getLastDiagnostic() noexcept
    {
        records_list_type result;
        swap(result, m_last_diagnostic_messages);
        return result;
    }

    static unsigned defaultPCHParseOptions();
    static unsigned defaultEditingParseOptions();
    static unsigned defaultExplorerParseOptions();

private:
    void updateDiagnostic();
    void appendDiagnostic(const CXDiagnostic&);
    static QString makeParentText(CXCompletionString, CXCursorKind);

    std::vector<std::pair<QByteArray, QByteArray>> m_unsaved_files_utf8;
    std::vector<CXUnsavedFile> m_unsaved_files;
    records_list_type m_last_diagnostic_messages;
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
