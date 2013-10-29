/**
 * \file
 *
 * \brief Class \c kate::IndexingTargetsListModel (interface)
 *
 * \date Thu Oct 24 01:13:50 MSK 2013 -- Initial design
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
#include <QtCore/QAbstractListModel>

namespace kate {
class DatabaseManager;                                      // fwd decl

/**
 * \brief A model class to prepresent a list of configured
 * targets to be indexed.
 *
 * Only \c DatabaseManager can make a new instances of this class.
 *
 */
class IndexingTargetsListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    /// Construct from a reference to \c DatabaseManager
    explicit IndexingTargetsListModel(DatabaseManager&);

    //BEGIN QAbstractListModel interface
    virtual int rowCount(const QModelIndex&) const override;
    virtual QVariant data(const QModelIndex&, int) const override;
    //END QAbstractListModel interface

    /// Add a new empty row
    template <typename AppendFunctor>
    void appendNewRow(int, AppendFunctor);

    /// Remove given row
    template <typename RemoveFunctor>
    void removeRow(const int, RemoveFunctor);

    /// Reset the model
    template <typename RefreshFunctor>
    void refreshAll(RefreshFunctor);

private:
    DatabaseManager& m_db_mgr;
};

template <typename AppendFunctor>
inline void IndexingTargetsListModel::appendNewRow(const int idx, AppendFunctor fn)
{
    beginInsertRows(QModelIndex(), idx, idx);
    fn();
    endInsertRows();
}

template <typename RemoveFunctor>
inline void IndexingTargetsListModel::removeRow(const int idx, RemoveFunctor fn)
{
    beginRemoveRows(QModelIndex(), idx, idx);
    fn(idx);
    endRemoveRows();
}

template <typename RefreshFunctor>
inline void IndexingTargetsListModel::refreshAll(RefreshFunctor fn)
{
    beginResetModel();
    fn();
    endResetModel();
}

}                                                           // namespace kate
