/**
 * \file
 *
 * \brief Class \c kate::utils (interface)
 *
 * \date Mon Feb  6 04:36:38 MSK 2012 -- Initial design
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

#pragma once

// Project specific includes

// Standard includes
#include <KDE/KTextEditor/Range>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QStringList>

namespace kate {

/// Enumerate possible ways to \c #include header
enum class IncludeStyle
{
    unknown                                                 ///< Pase errors seems occurs before complete
  , local                                                   ///< \c '"' used to \c #include a file
  , global                                                  ///< \c '<',\c '>' used to \c #include a file
};

/// Structure to hold \c #include parse results
struct IncludeParseResult
{
    KTextEditor::Range range;                               ///< A range w/ filename of \c #include directive
    /// \c #include type: local (w/ \c '""') or global (w/ \c '<>')
    IncludeStyle type;
    bool has_include;                                       ///< Is there \c #include on the line?
    bool is_complete;                                       ///< Is this valid \c #include directive?

    char open_char() const
    {
        return type == IncludeStyle::local
          ? '"'
          : type == IncludeStyle::global
              ? '<'
              : 0
          ;
    }
    char close_char() const
    {
        return type == IncludeStyle::local
          ? '"'
          : type == IncludeStyle::global
              ? '>'
              : 0
          ;
    }

    /// Default initializer: set state to \e invalid
    IncludeParseResult() : IncludeParseResult{KTextEditor::Range::invalid()}
    {
    }

    /// Initialize from range
    IncludeParseResult(const KTextEditor::Range& r)
      : range{r}
      , type{IncludeStyle::unknown}
      , has_include{false}
      , is_complete{false}
    {
    }
};

/// Get filename of \c #include directive at given line
IncludeParseResult parseIncludeDirective(const QString&, const bool);

/// \c true if given MIME type string belongs to C/C++ source or in case of
/// \c text-plain (set for new documents), check for highlighting mode
bool isSuitableDocument(const QString&, const QString&);
bool isSuitableDocumentAndHighlighting(const QString&, const QString&);

/**
 * \brief Try to find a file in a given location
 * \param[in] uri name of the file to lookup
 */
inline auto isPresentAndReadable(const QString& uri)
{
    const auto fi = QFileInfo{uri};
    return fi.exists() && fi.isFile() && fi.isReadable();
}

/**
 * \brief Find a given \c file in a list of \c paths and fill
 * \c result if file exists in a particular directory.
 */
template <typename Container>
inline void findFiles(const QString& file, Container&& paths, QStringList& result)
{
    for (const auto& path : paths)
    {
        const auto full_filename = QDir{QDir::cleanPath(path)}.absoluteFilePath(file);
        if (isPresentAndReadable(full_filename))
        {
            kDebug(DEBUG_AREA) << " ... " << full_filename << " Ok";
            result << full_filename;
        }
        else kDebug(DEBUG_AREA) << " ... " << full_filename << " doesn't exists/readable";
    }
}

/**
 * \brief Terminate recursion over containers in the templated
 * version of \c findHeader.
 */
inline auto findHeader(const QString&)
{
    return QStringList{};
}

/**
 * \brief Try to find a given filename in a list of paths
 *
 * \param[in] file filename to look for in the next 2 lists...
 * \param[in] head first container w/ paths to check
 * \param[in] others other containers w/ paths to check
 * \return list of absolute filenames
 *
 * \note \c QStringList used as a result because it has \c removeDuplicates().
 */
template <typename Container, typename... OtherContainers>
inline auto findHeader(
    const QString& file
  , Container&& head
  , OtherContainers&&... others
  )
{
    QStringList result;
    findFiles(file, std::forward<Container>(head), result);
    result << findHeader(file, std::forward<OtherContainers>(others)...);
    return result;
}

}                                                           // namespace kate
// kate: hl C++/Qt4;
