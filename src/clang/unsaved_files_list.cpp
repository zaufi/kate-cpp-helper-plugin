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

    auto it = m_index.find(file);
    if (it == end(m_index))                                 // Is it in a primary index already?
    {                                                       // (prevent double update)
        // No! Try to find it in a previous generation
        it = m_index_prev.find(file);
        if (it == end(m_index_prev))
        {
            // That is really new file: add content to the storage
            auto entry_it = m_unsaved_files.emplace(
                end(m_unsaved_files)
              , file.toLocalFile().toUtf8()
              , text.toUtf8()
              );
            // ... and update the primary index
            m_index.emplace(file, entry_it);
        }
        else
        {
            // Ok, lets set a new content to previously existed file
            auto tmp = text.toUtf8();
            it->second->second.swap(tmp);
            m_index.emplace(file, it->second);
        }
    }
    else
    {
        // Huh, file already in the new/updated index,
        // so lets update the content again
        auto tmp = text.toUtf8();
        it->second->second.swap(tmp);
    }
}

void unsaved_files_list::finalize_updating()
{
    for (auto it = begin(m_index_prev), last = end(m_index_prev); it != last; ++it)
    {
        auto existed_it = m_index.find(it->first);
        if (existed_it == end(m_index))
            m_unsaved_files.erase(it->second);
    }
    m_index_prev.clear();
    m_updating = false;
}

std::vector<CXUnsavedFile> unsaved_files_list::get() const
{
    assert("u must call finalize_updating() before!" && !m_updating);

    auto result = std::vector<CXUnsavedFile>{m_unsaved_files.size()};

    auto i = 0u;
    for (auto it = begin(m_unsaved_files), last = end(m_unsaved_files); it != last; ++it, ++i)
    {
        result[i].Filename = it->first.constData();
        result[i].Contents = it->second.constData();
        /// \note Fraking \c QByteArray has \c int as return type of \c size()! IDIOTS!?
        result[i].Length = unsigned(it->second.size());
    }
    assert("Sanity check" && i == result.size());
    return result;
}

void unsaved_files_list::initiate_updating()
{
    assert("Sanity check" && !m_updating && m_index_prev.empty());
    m_updating = true;
    swap(m_index, m_index_prev);
    assert("Sanity check" && m_updating && m_index.empty());
}

}}                                                          // namespace clang, kate
