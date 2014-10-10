/**
 * \file
 *
 * \brief Class \c kate::index::utils (interface)
 *
 * \date Sat Oct 26 12:53:39 MSK 2013 -- Initial design
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
#include "types.h"

// Standard includes
#include <boost/uuid/uuid.hpp>

class QString;

namespace kate { namespace index {

/// Helper function to produce (almost) random short DB id
dbid make_dbid(const boost::uuids::uuid&);

/// Helper function to parse UUID from \c QString
boost::uuids::uuid fromString(const QString&);

/// Helper function to render UUID to \c QString
QString toString(const boost::uuids::uuid&);

}}                                                          // namespace index, kate
