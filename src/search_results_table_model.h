/**
 * \file
 *
 * \brief Class \c kate::SearchResultsTableModel (interface)
 *
 * \date Mon Oct 28 09:34:39 MSK 2013 -- Initial design
 */
/*
 * Copyright (C) 2011-2013 Alex Turbov, all rights reserved.
 * This is free software. It is licensed for use, modification and
 * redistribution under the terms of the GNU General Public License,
 * version 3 or later <http://gnu.org/licenses/gpl.html>
 *
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
#include <src/index/kind.h>

// Standard includes
#include <boost/optional.hpp>
#include <clang-c/Index.h>
#include <QtCore/QAbstractTableModel>
#include <cassert>
#include <vector>

namespace kate {
class DatabaseManager;                                      // fwd decl

/**
 * \brief [Type brief class description here]
 *
 * [More detailed description here]
 *
 */
class SearchResultsTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    struct search_result
    {
        QString m_name;
        QString m_type;
        QString m_file;
        boost::optional<long long> m_value;
        int m_line = {-1};
        int m_column = {-1};
        index::kind m_kind;
        CXIdxEntityCXXTemplateKind m_template_kind = {CXIdxEntity_NonTemplate};
        bool m_static = {false};
        bool m_bit_field = {false};

        explicit search_result(const index::kind k)
          : m_kind{k}
        {}

        /// Delete copy ctor
        search_result(const search_result&) = delete;
        /// Delete copy-assign operator
        search_result& operator=(const search_result&) = delete;
        /// Move ctor
        search_result(search_result&&) noexcept;
        /// Move-assign operator
        search_result& operator=(search_result&&) noexcept;
        /// Get symbol kind as string
        QString kindSpelling() const;
    };
    typedef std::vector<search_result> search_results_list_type;

    /// Default constructor
    explicit SearchResultsTableModel(DatabaseManager&);

    //BEGIN QAbstractItemModel interface
    virtual int columnCount(const QModelIndex&) const override;
    virtual int rowCount(const QModelIndex&) const override;
    virtual QVariant data(const QModelIndex&, int) const override;
    virtual QVariant headerData(int, Qt::Orientation, int) const override;
    virtual Qt::ItemFlags flags(const QModelIndex&) const override;
    //END QAbstractItemModel interface

    void updateSearchResults(search_results_list_type&&);
    const search_result& getSearchResult(int) const;

private:
    enum column
    {
        KIND
      , NAME
      , last__
    };
    DatabaseManager& m_db_mgr;
    search_results_list_type m_results;
};

inline void SearchResultsTableModel::updateSearchResults(search_results_list_type&& results)
{
    beginResetModel();
    m_results = std::move(results);
    endResetModel();
}

/**
 * Get a search result by offset (index) in the internal collection
 *
 * \invariant \c row expected to be a valid index in the \c m_results
 */
inline auto SearchResultsTableModel::getSearchResult(const int row) const -> const search_result&
{
    assert("Sanity check" && std::size_t(row) < m_results.size());
    return m_results[row];
}

}                                                           // namespace kate
