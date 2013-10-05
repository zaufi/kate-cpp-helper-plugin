/**
 * \file
 *
 * \brief Class \c kate::HeaderFilesCache (interface)
 *
 * \date Thu Dec 27 14:25:24 MSK 2012 -- Initial design
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

#ifndef __SRC__HEADER_FILES_CACHE_H__
# define __SRC__HEADER_FILES_CACHE_H__

// Project specific includes

// Standard includes
# include <boost/multi_index_container.hpp>
# include <boost/multi_index/member.hpp>
# include <boost/multi_index/ordered_index.hpp>
# include <boost/multi_index/tag.hpp>
# include <boost/multi_index/indexed_by.hpp>
# include <QtCore/QString>

namespace kate {

/**
 * \brief Bidirectinal mapping of \c QString to unique integer ID
 *
 * To resolve a string into ID one can use \c operator[].
 * In case of absent (yet) string an unique ID will be assigned
 * and returned. To resolve ID into a string here is an
 * overload of \c operator[] w/ \c int parameter.
 */
class HeaderFilesCache
{
public:
    /// \name Accessors
    //@{
    const QString operator[](int id) const;                 ///< Operator to get a string value by ID
    int operator[](const QString&) const ;                  ///< Operator to get an ID by string
    int operator[](const QString&);                         ///< Operator to get an ID by string
    //@}

    enum : int
    {
        NOT_FOUND = -1
    };

    bool isEmpty() const;
    size_t size() const;

private:
    /**
     * \brief Structure to associate a filename with an index
     * \todo Not sure that \c QString is a good idea to store
     */
    struct value_type
    {
        QString m_filename;
        int m_id;
    };
    struct int_idx;
    struct string_idx;
    /// Type to hold index of header files and assigned IDs
    typedef boost::multi_index_container<
        value_type
      , boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<int_idx>
              , boost::multi_index::member<
                    value_type
                  , decltype(value_type::m_id)
                  , &value_type::m_id
                  >
              >
          , boost::multi_index::ordered_unique<
                boost::multi_index::tag<string_idx>
              , boost::multi_index::member<
                    value_type
                  , decltype(value_type::m_filename)
                  , &value_type::m_filename
                  >
              >
          >
      > index_type;

    index_type m_cache;
};

/**
 * \param[in] id identifier to get filename for
 * \return filename for correcponding ID, or empty string if not found
 */
inline const QString HeaderFilesCache::operator[](int id) const
{
    QString result;
    auto it = m_cache.get<int_idx>().find(id);
    if (it != end(m_cache.get<int_idx>()))
        result = it->m_filename;
    return result;
}

/**
 * Immutable version of ID access operator.
 *
 * \param[in] filename a filename to get identifier for
 * \return ID of the given value or \c HeaderFilesCache::NOT_FOUND
 */
inline int HeaderFilesCache::operator[](const QString& filename) const
{
    auto result = int(NOT_FOUND);
    auto it = m_cache.get<string_idx>().find(filename);
    if (it != end(m_cache.get<string_idx>()))
    {
        result = it->m_id;
    }
    return result;
}

/**
 * Mutable version of ID access operator.
 *
 * \param[in] filename a filename to get identifier for
 * \return ID of the given value (will add a new cache entry if not found)
 */
inline int HeaderFilesCache::operator[](const QString& filename)
{
    auto result = int(NOT_FOUND);
    auto it = m_cache.get<string_idx>().find(filename);
    if (it == end(m_cache.get<string_idx>()))
    {
        result = int(m_cache.size());
        m_cache.insert({filename, result});
    }
    else result = it->m_id;
    return result;
}

inline bool HeaderFilesCache::isEmpty() const
{
    return m_cache.empty();
}
inline size_t HeaderFilesCache::size() const
{
    return m_cache.size();
}

}                                                           // namespace kate
#endif                                                      // __SRC__HEADER_FILES_CACHE_H__
// kate: hl C++11/Qt4;
