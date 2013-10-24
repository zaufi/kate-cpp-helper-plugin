/**
 * \file
 *
 * \brief Class \c kate::DatabaseManager (interface)
 *
 * \date Sun Oct 13 08:47:24 MSK 2013 -- Initial design
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
#include <src/index/database.h>
#include <src/database_options.h>
#include <src/diagnostic_messages_model.h>

// Standard includes
#include <boost/filesystem/path.hpp>
#include <KDE/KUrl>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

class QAbstractTableModel;
class QAbstractListModel;
class QModelIndex;

namespace kate {
class IndicesTableModel;                                    // fwd decls
class IndexingTargetsListModel;

/**
 * \brief Manage databases used by current session
 *
 * [More detailed description here]
 *
 */
class DatabaseManager
  : public QObject
  , public std::enable_shared_from_this<DatabaseManager>
{
    Q_OBJECT

public:
    /// Construct from default base directory and list of enabled colelctions
    explicit DatabaseManager(const QStringList&);
    /// Construct w/ base path specified and list of enabled collections
    explicit DatabaseManager(const KUrl&, const QStringList&);
    /// Destructor
    ~DatabaseManager();

    struct exception : public std::runtime_error
    {
        struct invalid_manifest;
        explicit exception(const std::string&);
    };

    /// Obtain a table model for currently configured indices
    QAbstractTableModel* getDatabasesTableModel();
    /// Obtain a list model for currently configured targets
    QAbstractListModel* getTargetsListModel();

public Q_SLOTS:
    void enable(const QString&, bool);
    void createNewIndex();
    void removeCurrentIndex();
    void rebuildCurrentIndex();
    void refreshCurrentTargets(const QModelIndex&);
    void selectCurrentTarget(const QModelIndex&);
    void addNewTarget();
    void removeCurrentTarget();

Q_SIGNALS:
    void indexStatusChanged(const QString&, bool);
    void indexNameChanged(const QString&, const QString&);
    void diagnosticMessage(DiagnosticMessagesModel::Record);

private:
    friend class IndicesTableModel;
    friend class IndexingTargetsListModel;

    struct database_state
    {
        enum class status
        {
            unknown
          , ok
          , invalid
          , reindexing
        };
        std::unique_ptr<DatabaseOptions> m_options;
        std::unique_ptr<index::ro::database> m_db;
        status m_status = {status::unknown};
        bool m_enabled = {false};

        database_state() = default;                         ///< Default initialization
        database_state(database_state&&) = default;         ///< Default move constructor
        /// Default move-assign operator
        database_state& operator=(database_state&&) = default;

        bool isOk() const;
    };

    typedef std::vector<database_state> collections_type;

    static KUrl getDefaultBaseDir();
    database_state tryLoadDatabaseMeta(const boost::filesystem::path&);
    void enable(int, bool);
    bool isEnabled(int) const;
    void renameCollection(int, const QString&);

    const KUrl m_base_dir;
    collections_type m_collections;
    QStringList m_enabled_list;
    std::unique_ptr<IndicesTableModel> m_indices_model;
    std::unique_ptr<IndexingTargetsListModel> m_targets_model;
    int m_last_selected_index;
    int m_last_selected_target;
};

struct DatabaseManager::exception::invalid_manifest : public DatabaseManager::exception
{
    invalid_manifest(const std::string& str) : DatabaseManager::exception(str) {}
};

inline DatabaseManager::exception::exception(const std::string& str)
  : std::runtime_error(str)
{}

inline DatabaseManager::DatabaseManager(const QStringList& enabled_list)
  : DatabaseManager(DatabaseManager::getDefaultBaseDir(), enabled_list)
{
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
