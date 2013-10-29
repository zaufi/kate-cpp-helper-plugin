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

// Standard includes
#include <QtCore/QAbstractTableModel>

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
    /// Default constructor
    explicit SearchResultsTableModel(DatabaseManager&);

    //BEGIN QAbstractItemModel interface
    virtual int columnCount(const QModelIndex&) const override;
    virtual int rowCount(const QModelIndex&) const override;
    virtual QVariant data(const QModelIndex&, int) const override;
    virtual QVariant headerData(int, Qt::Orientation, int) const override;
    virtual Qt::ItemFlags flags(const QModelIndex&) const override;
    //END QAbstractItemModel interface

private:
    enum column
    {
        LOCATION
      , NAME
      , last__
    };
    DatabaseManager& m_db_mgr;
};

}                                                           // namespace kate
