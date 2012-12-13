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

#ifndef __SRC__UTILS_H__
# define __SRC__UTILS_H__

// Project specific includes

// Standard includes
// # include <KUrl>
# include <KTextEditor/Range>
# include <QtCore/QDir>
# include <QtCore/QFileInfo>
# include <QtCore/QStringList>

namespace kate {
struct IncludeStyle
{
    enum type
    {
        unknown                                             ///< Pase errors seems occurs before complete
      , local                                               ///< \c '"' used to \c #include a file
      , global                                              ///< \c '<',\c '>' used to \c #include a file
    };
};

struct IncludeParseResult
{
    KTextEditor::Range m_range;                             ///< A range w/ filename of \c #include directive
    IncludeStyle::type m_type;
    bool m_has_include;                                     ///< Is there \c #include on the line?
    bool m_is_complete;                                     ///< Is this valid \c #include directive?

    char open_char() const
    {
        return m_type == IncludeStyle::local
          ? '"'
          : m_type == IncludeStyle::global
              ? '<'
              : 0
          ;
    }
    char close_char() const
    {
        return m_type == IncludeStyle::local
          ? '"'
          : m_type == IncludeStyle::global
              ? '>'
              : 0
          ;
    }
};

/// Get filename of \c #include directive at given line
IncludeParseResult parseIncludeDirective(const QString&, const bool);

/// \c true if given MIME type string belongs to C/C++ source
inline bool isCOrPPSource(const QString& mime_str)
{
    return (mime_str == "text/x-c++src")
      || (mime_str == "text/x-c++hdr")
      || (mime_str == "text/x-csrc")
      || (mime_str == "text/x-chdr")
      ;
}

/**
 * \brief Remove duplicates from strings list
 * \return \b sorted and deduplicated list
 * \todo Is there analog of \c std::unique?
 */
inline void removeDuplicates(QStringList& list)
{
    list.sort();
    for (
        QStringList::iterator it = list.begin()
      , prev = list.end()
      , last = list.end()
      ; it != last
      ;
      )
    {
        if (prev != last && *it == *prev) list.erase(it++);
        else prev = it++;
    }
}

/**
 * \brief Try to find a file in a given location
 * \param[in] uri name of the file to lookup
 */
inline bool isPresentAndReadable(const QString& uri)
{
    const QFileInfo fi = QFileInfo(uri);
    return fi.exists() && fi.isFile() && fi.isReadable();
}

inline void findFiles(const QString& file, const QStringList& paths, QStringList& result)
{
    for (const QString& path : paths)
    {
        const QString full_filename = QDir::cleanPath(path + '/' + file);
        if (isPresentAndReadable(full_filename))
        {
            result.push_back(full_filename);
            kDebug() << " ... " << full_filename << " Ok";
        }
        else kDebug() << " ... " << full_filename << " not exists/readable";
    }
}

/// Scan for files and dirs
void updateListsFromFS(
    const QString&
  , const QStringList&
  , const QStringList&
  , QStringList&
  , QStringList&
  );

/// Find given header withing list of paths
QStringList findHeader(const QString&, const QStringList&, const QStringList&);
}                                                           // namespace kate
#endif                                                      // __SRC__UTILS_H__
// kate: hl C++11/Qt4;
