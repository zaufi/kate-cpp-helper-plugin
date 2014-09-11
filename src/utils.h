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

enum class IncludeStyle
{
    unknown                                                 ///< Pase errors seems occurs before complete
  , local                                                   ///< \c '"' used to \c #include a file
  , global                                                  ///< \c '<',\c '>' used to \c #include a file
};

struct IncludeParseResult
{
    KTextEditor::Range range;                               ///< A range w/ filename of \c #include directive
    /// \c #include type: local (w/ \c '""' or global (w/ \c '<>')
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
    IncludeParseResult()
      : range{KTextEditor::Range::invalid()}
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

inline void findFiles(const QString& file, const QStringList& paths, QStringList& result)
{
    for (const auto& path : paths)
    {
        const auto full_filename = QDir::cleanPath(path + '/' + file);
        if (isPresentAndReadable(full_filename))
        {
            kDebug(DEBUG_AREA) << " ... " << full_filename << " Ok";
            result.push_back(std::move(full_filename));
        }
        else kDebug(DEBUG_AREA) << " ... " << full_filename << " doesn't exists/readable";
    }
}

/// Scan for files and dirs
void updateListsFromFS(
    const QString&
  , const QStringList&
  , const QStringList&
  , QStringList&
  , QStringList&
  , const QStringList&
  );

/// Find given header withing list of paths
QStringList findHeader(const QString&, const QStringList&, const QStringList&);

}                                                           // namespace kate
// kate: hl C++/Qt4;
