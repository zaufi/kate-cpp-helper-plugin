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

// Standard includes
#include <KDE/KUrl>

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
    database_manager();                                     ///< Construct from default base directory
    explicit database_manager(const KUrl&);                 ///< Construct w/ base path specified
    ~database_manager();                                    ///< Destructor

private:
    static KUrl get_default_base_dir();
};

inline database_manager::database_manager()
  : database_manager(database_manager::get_default_base_dir())
{
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
