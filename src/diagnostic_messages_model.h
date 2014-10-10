/**
 * \file
 *
 * \brief Class \c kate::DiagnosticMessagesModel (interface)
 *
 * \date Sun Aug 11 04:42:02 MSK 2013 -- Initial design
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
#include "clang/diagnostic_message.h"

// Standard includes
#include <QtCore/QAbstractListModel>
#include <deque>

namespace kate {

/**
 * \brief A mode class to hold diagnostic messages
 *
 * Data from this class will be displayed in the <em>Diagnostic Messages</em>
 * tab of the plugin sidebar.
 *
 */
class DiagnosticMessagesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    /// Default constructor
    DiagnosticMessagesModel() = default;

    clang::location getLocationByIndex(const QModelIndex&) const;

    /// \name QAbstractTableModel interface
    //@{
    /// Get rows count
    virtual int rowCount(const QModelIndex& = QModelIndex()) const override;
    /// Get columns count
    virtual int columnCount(const QModelIndex& = QModelIndex()) const override;
    /// Get data for given index and role
    virtual QVariant data(const QModelIndex&, int = Qt::DisplayRole) const override;
    //@}

    /// \name Records manipulation
    //@{
    /// Append a diagnostic record to the model
    void append(clang::diagnostic_message&&);
    /// Append a diagnostic record to the model (bulk version)
    template <typename Iter>
    void append(Iter, Iter);
    //@}

public Q_SLOTS:
    void clear();

private:
    std::deque<clang::diagnostic_message> m_records;        ///< Stored records
};

inline clang::location DiagnosticMessagesModel::getLocationByIndex(const QModelIndex& index) const
{
    return m_records[index.row()].m_location;
}

/**
 * \attention Qt4 has a problem w/ rvalue references in signal/slots ;(
 */
inline void DiagnosticMessagesModel::append(clang::diagnostic_message&& record)
{
    beginInsertRows(QModelIndex(), m_records.size(), m_records.size());
    m_records.emplace_back(std::move(record));
    endInsertRows();
}

template <typename Iter>
inline void DiagnosticMessagesModel::append(Iter first, Iter last)
{
    beginInsertRows(
        QModelIndex()
      , m_records.size()
      , m_records.size() + std::distance(first, last) - 1
      );
    m_records.insert(end(m_records), first, last);
    endInsertRows();
}

}                                                           // namespace kate
// kate: hl C++/Qt4;
