/**
 * \file
 *
 * \brief Class \c kate::index::numeric_value_range_processor (interface)
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

#pragma once

// Project specific includes
#include <src/index/database.h>
#include <src/index/types.h>

// Standard includes
#include <xapian/queryparser.h>

namespace kate { namespace index {

/**
 * \brief Custom value range processor for Xapian engine
 *
 * Implement a numeric range parser w/ prefix/suffix support.
 * A numeric parser shipped w/ Xapian can't have prefix and suffix
 * at the same time... But we want to have such a feature!
 *
 */
class numeric_value_range_processor : public Xapian::ValueRangeProcessor
{
  public:
    /// Default constructor
    explicit numeric_value_range_processor(
        const value_slot slot
      , const std::string& prefix = std::string()
      , const std::string& value_prefix = std::string()
      , const std::string& value_suffix = std::string()
      )
      : m_prefix{prefix + ":"}
      , m_value_prefix{value_prefix}
      , m_value_suffix{value_suffix}
      , m_slot{slot}
    {}

    virtual Xapian::valueno operator()(std::string&, std::string&) override;

  private:
    const std::string m_prefix;
    const std::string m_value_prefix;
    const std::string m_value_suffix;
    const value_slot m_slot;
};

}}                                                          // namespace index, kate
