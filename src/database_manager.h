/**
 * \file
 *
 * \brief Class \c kate::database_manager (interface)
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

// Standard includes
#include <boost/filesystem/path.hpp>
#include <KDE/KUrl>
#include <QtCore/QStringList>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace kate {

/**
 * \brief Manage databases used by current session
 *
 * [More detailed description here]
 *
 */
class database_manager
{
public:
    /// Construct from default base directory and list of enabled colelctions
    explicit database_manager(const QStringList&);
    /// Construct w/ base path specified and list of enabled collections
    explicit database_manager(const KUrl&, const QStringList&);
    /// Destructor
    ~database_manager();

    struct exception : public std::runtime_error
    {
        struct invalid_manifest;
        explicit exception(const std::string&);
    };

public Q_SLOTS:
    void enable(const QStringList&);

private:
    struct database_options
    {
        enum class status
        {
            invalid
          , enabled
          , disabled
          , reindexing
        };
        KUrl m_path;
        QStringList m_targets;
        QString m_name;
        QString m_comment;
        status m_status = {status::invalid};

        database_options() = default;
        database_options(database_options&&) noexcept;
        database_options& operator=(database_options&&) noexcept;
    };
    static KUrl get_default_base_dir();
    database_options try_load_database_meta(const boost::filesystem::path&);

    typedef std::vector<std::pair<database_options, std::unique_ptr<index::database>>> collections_type;
    collections_type m_collections;
    QStringList m_enabled_list;
};

struct database_manager::exception::invalid_manifest : public database_manager::exception
{
    invalid_manifest(const std::string& str) : database_manager::exception(str) {}
};

inline database_manager::exception::exception(const std::string& str)
  : std::runtime_error(str) {}

inline database_manager::database_manager(const QStringList& enabled_list)
  : database_manager(database_manager::get_default_base_dir(), enabled_list)
{
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
