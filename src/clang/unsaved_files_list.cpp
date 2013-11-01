/**
 * \file
 *
 * \brief Class \c kate::clang::unsaved_files_list (implementation)
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

// Project specific includes
#include <src/clang/unsaved_files_list.h>

// Standard includes
#include <KDE/KDebug>
#include <cassert>

namespace kate { namespace clang {

void unsaved_files_list::update(const KUrl& file, const QString& text)
{
    if (!m_updating)
        initiate_updating();

    assert("Sanity check" && m_updating);
    assert("Sanity check" && !file.toLocalFile().isEmpty());

    auto it = m_index_prev.find(file);
    if (it == end(m_index_prev))
    {
        auto entry_it = m_unsaved_files.emplace(
            end(m_unsaved_files)
          , file.toLocalFile().toUtf8()
          , text.toUtf8()
          );
        m_index.emplace(file, entry_it);
    }
    else
    {
        auto tmp = text.toUtf8();
        it->second->second.swap(tmp);
        m_index.emplace(file, it->second);
    }
}

void unsaved_files_list::finalize_updating()
{
    for (auto it = begin(m_index_prev), last = end(m_index_prev); it != last; ++it)
    {
        auto existed_it = m_index.find(it->first);
        if (existed_it == end(m_index))
        {
            kDebug(DEBUG_AREA) << "UF Cache: Removing" << it->first;
            m_unsaved_files.erase(it->second);
        }
    }
    m_index_prev.clear();
    m_updating = false;
}

std::vector<CXUnsavedFile> unsaved_files_list::get() const
{
    std::vector<CXUnsavedFile> result;
    result.resize(m_unsaved_files.size());

    auto i = 0u;
    for (auto it = begin(m_unsaved_files), last = end(m_unsaved_files); it != last; ++it)
    {
        result[i].Filename = it->first.constData();
        result[i].Contents = it->second.constData();
        /// \note Fraking \c QByteArray has \c int as return type of \c size()! IDIOTS!
        result[i].Length = unsigned(it->second.size());
    }
    return result;
}

void unsaved_files_list::initiate_updating()
{
    assert("Sanity check" && !m_updating && m_index_prev.empty());
    m_updating = true;
    swap(m_index, m_index_prev);
}

}}                                                          // namespace clang, kate
