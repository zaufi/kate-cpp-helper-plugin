/**
 * \file
 *
 * \brief Class \c kate::indexer_tester (interface)
 *
 * \date Fri Oct 11 02:29:31 MSK 2013 -- Initial design
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
#include <src/index/indexer.h>

// Standard includes
#include <QtCore/QObject>
#include <atomic>

namespace kate {

/**
* \brief Class \c result_waiter
*/
class result_waiter : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void finished();

public:
    std::atomic<bool> m_done;
};


/**
 * \brief [Type brief class description here]
 *
 * [More detailed description here]
 *
 */
class indexer_tester : public QObject
{
    Q_OBJECT

public:
    indexer_tester();

private Q_SLOTS:
    void index_sample_file();

private:
    index::indexer m_indexer;
    result_waiter m_res;
};

}                                                           // namespace kate
