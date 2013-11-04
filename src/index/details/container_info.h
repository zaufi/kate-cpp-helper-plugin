/**
 * \file
 *
 * \brief Class \c kate::index::details::container_info (interface)
 *
 * \date Sun Nov  3 19:01:30 MSK 2013 -- Initial design
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
#include <src/index/types.h>

// Standard includes
#include <string>

namespace kate { namespace index { namespace details {

/**
 * \brief [Type brief class description here]
 *
 * [More detailed description here]
 *
 */
struct container_info
{
    std::string m_name;                                     ///< Entity name
    std::string m_qname;                                    ///< Fully qualified name
    docref m_ref;                                           ///< ref to a container ID in a current DB

    container_info(docref ref, const std::string& name, const std::string& qname)
      : m_name{name}
      , m_qname{qname}
      , m_ref{ref}
    {}
    /// Default move ctor
    container_info(container_info&&) = default;
    /// Default move-assign operator
    container_info& operator=(container_info&&) = default;
};

}}}                                                         // namespace details, index, kate
