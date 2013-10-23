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
    explicit IndicesTableModel(const std::weak_ptr<DatabaseManager>&);

    //BEGIN QAbstractItemModel interface
    virtual int rowCount(const QModelIndex&) const final override;
    virtual int columnCount(const QModelIndex&) const final override;
    virtual QVariant data(const QModelIndex&, int) const final override;
#if 0
    virtual QVariant headerData(int, Qt::Orientation, int) const final override;
#endif
    virtual Qt::ItemFlags flags(const QModelIndex&) const final override;
    virtual bool setData(const QModelIndex&, const QVariant&, int) final override;
    //END QAbstractItemModel interface

private:
    enum class column
    {
        STATUS
      , NAME
      , last__
    };
    std::weak_ptr<DatabaseManager> m_db_mgr;
};

}                                                           // namespace kate
