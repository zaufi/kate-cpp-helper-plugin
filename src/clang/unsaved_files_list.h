/**
 * \file
 *
 * \brief Class \c kate::clang::unsaved_files_list (interface)
 *
 * \date Fri Oct 25 05:40:21 MSK 2013 -- Initial design
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
#include <clang-c/Index.h>
#include <KDE/KUrl>
#include <deque>
#include <list>
#include <map>
#include <vector>
#include <utility>

class QString;

namespace kate { namespace clang {

/**
 * \brief A wrapper class to hide ugly underlaid details of unsaved
 * files list maintanance.
 *
 * This class actually acts like a cache ;)
 *
 * \sa About origin of the problem: \c kate::clang::compiler_options
 *
 */
class unsaved_files_list
{
public:
    /// Defaulted default constructor
    unsaved_files_list() = default;
    /// Default move ctor
    unsaved_files_list(unsaved_files_list&&) = default;
    /// Default move-assign operator
    unsaved_files_list& operator=(unsaved_files_list&&) = default;
    /// Delete copy ctor
    unsaved_files_list(const unsaved_files_list&) = delete;
    /// Delete copy-assign operator
    unsaved_files_list& operator=(const unsaved_files_list&) = delete;

    /// Update unsaved file
    void update(const KUrl&, const QString&);
    /// No more updates are coming...
    void finalize_updating();

    /// Get a list of unsaved files in clang-c acceptable format
    std::vector<CXUnsavedFile> get() const;

private:
    typedef std::list<std::pair<QByteArray, QByteArray>> list_type;
    typedef std::map<KUrl, list_type::iterator> uri_index_type;

    void initiate_updating();

    list_type m_unsaved_files;
    uri_index_type m_index;
    uri_index_type m_index_prev;
    bool m_updating = {false};
};

}}                                                          // namespace clang, kate
