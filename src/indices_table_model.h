/**
 * \file
 *
 * \brief Class \c kate::IndicesTableModel (interface)
 *
 * \date Tue Oct 22 16:08:11 MSK 2013 -- Initial design
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
#include <memory>

namespace kate {
class DatabaseManager;                                      // fwd decl

/**
 * \brief A model class to represent a table w/ configured indices
 * used in a current session
 *
 * Only \c DatabaseManager can make a new instances of this class
 *
 */
class IndicesTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    /// Construct from a weak pointer to \c DatabaseManager
    explicit IndicesTableModel(DatabaseManager&);

    //BEGIN QAbstractItemModel interface
    virtual int columnCount(const QModelIndex&) const override;
    virtual int rowCount(const QModelIndex&) const override;
    virtual QVariant data(const QModelIndex&, int) const override;
#if 0
    virtual QVariant headerData(int, Qt::Orientation, int) const override;
#endif
    virtual Qt::ItemFlags flags(const QModelIndex&) const override;
    virtual bool setData(const QModelIndex&, const QVariant&, int) override;
    //END QAbstractItemModel interface

    template <typename AppendFunctor>
    void appendNewRow(int, AppendFunctor);                  ///< Add a new empty row

private:
    enum column
    {
        NAME
      , last__
    };

    DatabaseManager& m_db_mgr;
};

template <typename AppendFunctor>
inline void IndicesTableModel::appendNewRow(const int idx, AppendFunctor fn)
{
    beginInsertRows(QModelIndex(), idx, idx);
    fn();
    endInsertRows();
}

}                                                           // namespace kate
