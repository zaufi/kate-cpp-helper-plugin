/**
 * \file
 *
 * \brief Class \c kate::index::indexer (implementation)
 *
 * \date Wed Oct  9 11:17:12 MSK 2013 -- Initial design
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
#include <src/index/indexer.h>
#include <src/index/details/worker.h>

// Standard includes
#include <KDE/KDebug>
#include <QtCore/QThread>

namespace kate { namespace index {

indexer::indexer(const dbid id, const std::string& path)
  : m_index{clang_createIndex(1, 1)}
  , m_db{id, path}
{
}

indexer::~indexer()
{
    if (m_worker_thread)
    {
        stop();
        m_worker_thread->quit();
        m_worker_thread->wait();
    }
}

void indexer::start()
{
    auto* t = new QThread{};
    auto* w = new details::worker{this};
    connect(w, SIGNAL(error(clang::location, QString)), this, SLOT(error_slot(clang::location, QString)));
    connect(w, SIGNAL(indexing_uri(QString)), this, SLOT(indexing_uri_slot(QString)));
    connect(w, SIGNAL(finished()), this, SLOT(finished_slot()));
    connect(this, SIGNAL(stopping()), w, SLOT(request_cancel()));

    connect(w, SIGNAL(finished()), t, SLOT(quit()));
    connect(w, SIGNAL(finished()), w, SLOT(deleteLater()));

    connect(t, SIGNAL(started()), w, SLOT(process()));
    connect(t, SIGNAL(finished()), t, SLOT(deleteLater()));
    w->moveToThread(t);
    t->start();
    m_worker_thread.reset(t);
}

void indexer::stop()
{
    kDebug(DEBUG_AREA) << "Emitting STOP!";
    Q_EMIT(stopping());
}

void indexer::error_slot(clang::location loc, QString str)
{
    Q_EMIT(error(loc, str));
}

void indexer::finished_slot()
{
    m_worker_thread = nullptr;
    Q_EMIT(finished());
}

void indexer::indexing_uri_slot(QString file)
{
    Q_EMIT(indexing_uri(file));
}

}}                                                          // namespace index, kate
