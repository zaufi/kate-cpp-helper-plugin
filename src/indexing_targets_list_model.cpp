/**
 * \file
 *
 * \brief Class \c kate::IndexingTargetsListModel (implementation)
 *
 * \date Thu Oct 24 01:13:50 MSK 2013 -- Initial design
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
#include <src/indexing_targets_list_model.h>
#include <src/database_manager.h>

// Standard includes

namespace kate {

IndexingTargetsListModel::IndexingTargetsListModel(DatabaseManager& db_mgr)
  : QAbstractListModel(nullptr)
  , m_db_mgr(db_mgr)
{
}

int IndexingTargetsListModel::rowCount(const QModelIndex&) const
{
    if (m_db_mgr.m_last_selected_index != -1)
    {
        assert("Sanity check" && std::size_t(m_db_mgr.m_last_selected_index) < m_db_mgr.m_collections.size());
        return m_db_mgr.m_collections[m_db_mgr.m_last_selected_index].m_options->targets().size();
    }
    return 0;
}

QVariant IndexingTargetsListModel::data(const QModelIndex& index, const int role) const
{
    if (m_db_mgr.m_last_selected_index != -1 && role == Qt::DisplayRole)
    {
        const auto& targets = m_db_mgr.m_collections[m_db_mgr.m_last_selected_index].m_options->targets();
        assert("Sanity check" && index.row() < targets.size());
        return targets[index.row()];
    }
    return QVariant{};
}

}                                                           // namespace kate
