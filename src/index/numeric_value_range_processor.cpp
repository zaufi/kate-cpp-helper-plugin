/**
 * \file
 *
 * \brief Class \c kate::index::numeric_value_range_processor (implementation)
 *
 * \date Sun Nov  3 01:32:00 MSK 2013 -- Initial design
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

// Project specific includes
#include "numeric_value_range_processor.h"

// Standard includes
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

namespace kate { namespace index {

Xapian::valueno numeric_value_range_processor::operator()(std::string& start, std::string& end)
{
    auto is_valid = true;                                   // Assume everything would be fine :)
    // Is prefix specified?
    if (!m_prefix.empty())
    {
        // Yep, Does it matches?
        if (m_prefix.size() < start.size() && boost::starts_with(start, m_prefix))
            boost::erase_head(start, m_prefix.size());      // Erase prefix from start
        else
            is_valid = false;
    }
    // Is value-prefix specified?
    if (is_valid && !m_value_prefix.empty())
    {
        // Yeah, does it matches?
        if (m_value_prefix.size() < start.size() && boost::starts_with(start, m_value_prefix))
        {
            boost::erase_head(start, m_value_prefix.size());// Erase value-prefix from start
            // Possible end value specified w/ prefix as well...
            if (boost::starts_with(end, m_value_prefix))
                // Erase value-prefix from end
                boost::erase_head(end, m_value_prefix.size());
        }
        else is_valid = false;
    }
    // Is value-suffix specified?
    if (is_valid && !m_value_suffix.empty())
    {
        // Yeah, does it matches?
        if (m_value_suffix.size() < end.size() && boost::ends_with(end, m_value_suffix))
        {
            boost::erase_tail(end, m_value_suffix.size());  // Erase value-suffix from end
            // Possible start value specified w/ prefix as well...
            if (boost::ends_with(start, m_value_suffix))
                // Erase value-suffix from start
                boost::erase_tail(start, m_value_suffix.size());
        }
        else is_valid = false;
    }

    if (is_valid)
    {
        try
        {
            // Convert string to doubles
            const auto start_value = boost::lexical_cast<double>(start);
            const auto end_value = boost::lexical_cast<double>(end);
            // Reassign!
            // ATTENTION Fracking Xapian's documentation tells nothing about this!
            start = Xapian::sortable_serialise(start_value);
            end = Xapian::sortable_serialise(end_value);
        }
        catch (...)
        {
            is_valid = false;
            /// \todo Make some SPAM!
        }
    }

    return is_valid ? Xapian::valueno(m_slot) : Xapian::BAD_VALUENO;
}

}}                                                          // namespace index, kate
