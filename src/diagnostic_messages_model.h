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

#ifndef __SRC__DIAGNOSTIC_MESSAGES_MODEL_HH__
# define __SRC__DIAGNOSTIC_MESSAGES_MODEL_HH__

// Project specific includes
# include <src/clang_utils.h>

// Standard includes
# include <QAbstractListModel>
# include <deque>

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
    /**
     * Structure to hold info about diagnostic message
     *
     * \note Qt4 has a problem w/ rvalue references in signal/slots,
     * so default copy ctor and assign operator needed :(
     */
    struct Record
    {
        enum class type
        {
            debug
          , info
          , warning
          , error
          , cutset
        };
        location m_location;                                ///< Location in source code
        QString m_text;                                     ///< Diagnostic message text
        type m_type;                                        ///< Type of the record

        /// \c record class must be default constructible
        Record() = default;
        /// Make a \c record from parts
        Record(location&&, QString&&, type) noexcept;
        /// Make a \c record w/ message and given type (and empty location)
        Record(QString&&, type) noexcept;
        /// Move ctor
        Record(Record&&) noexcept;
        /// Move-assign operator
        Record& operator=(Record&&) noexcept;
        /// Default copy ctor
        Record(const Record&) = default;
        /// Default copy-assign operator
        Record& operator=(const Record&) = default;
    };

    /// Default constructor
    DiagnosticMessagesModel() = default;

    location getLocationByIndex(const QModelIndex&) const;

    /// \name QAbstractTableModel interface
    //@{
    /// Get rows count
    int rowCount(const QModelIndex& = QModelIndex()) const;
    /// Get columns count
    int columnCount(const QModelIndex& = QModelIndex()) const;
    /// Get data for given index and role
    QVariant data(const QModelIndex&, int = Qt::DisplayRole) const;
    //@}

    /// \name Records manipulation
    //@{
    /// Append a diagnostic record to the model
    void append(Record&&);
    /// Append a diagnostic record to the model (bulk version)
    template <typename Iter>
    void append(Iter, Iter);
    //@}

public Q_SLOTS:
    void clear();

private:
    std::deque<Record> m_records;                           ///< Stored records
};

inline DiagnosticMessagesModel::Record::Record(QString&& text, Record::type type) noexcept
  : m_type(type)
{
    m_text.swap(text);
}

inline DiagnosticMessagesModel::Record::Record(
    location&& loc
  , QString&& text
  , const Record::type type
  ) noexcept
  : m_location(std::move(loc))
  , m_type(type)
{
    m_text.swap(text);
}

inline DiagnosticMessagesModel::Record::Record(Record&& other) noexcept
  : m_location(std::move(other.m_location))
  , m_type(other.m_type)
{
    m_text.swap(other.m_text);
}

inline auto DiagnosticMessagesModel::Record::operator=(Record&& other) noexcept -> Record&
{
    if (&other != this)
    {
        m_location = std::move(other.m_location);
        m_text.swap(other.m_text);
        m_type = other.m_type;
    }
    return *this;
}


inline location DiagnosticMessagesModel::getLocationByIndex(const QModelIndex& index) const
{
    return m_records[index.row()].m_location;
}

/**
 * \attention Qt4 has a problem w/ rvalue references in signal/slots ;(
 */
inline void DiagnosticMessagesModel::append(Record&& record)
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
#endif                                                      // __SRC__DIAGNOSTIC_MESSAGES_MODEL_HH__
