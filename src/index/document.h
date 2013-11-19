/**
 * \file
 *
 * \brief Class \c kate::index::document (interface)
 *
 * \date Sun Nov  3 19:40:26 MSK 2013 -- Initial design
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
#include <src/index/document_extras.h>

// Standard includes

namespace kate { namespace index {

/**
 * \brief Class \c document
 */
class document : public Xapian::Document
{
public:
    /// Defaulted default ctor
    document() = default;
    /// Conversion ctor from \c Xapian::Document
    document(Xapian::Document d) : Xapian::Document{d} {}
    /// Default copy ctor
    document(const document&) = default;
    /// Default copy-assign operator
    document& operator=(const document&) = default;

    void add_value(const value_slot slot, const std::string& value)
    {
        Xapian::Document::add_value(Xapian::valueno(slot), value);
    }
    std::string get_value(const value_slot slot) const
    {
        return Xapian::Document::get_value(Xapian::valueno(slot));
    }
};

}}                                                          // namespace index, kate
