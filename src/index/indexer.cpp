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

// Standard includes
#include <KDE/KDebug>
#include <unistd.h>

namespace kate { namespace index {

worker::worker(indexer* const parent)
  : m_indexer(parent)
{
}

void worker::cancel_request()
{
    m_is_cancelled = true;
}

bool worker::is_cancelled() const
{
    return m_is_cancelled;
}

void worker::process()
{
    kDebug() << "STARTED: Going to sleep...";
    for (int i = 0; i < 2; ++i)
    {
        kDebug() << "Sleeping" << i << "...";
        sleep(1);
    }
    kDebug() << "Emitting a signal";
    Q_EMIT(finished());
}

void indexer::start()
{
    QThread* t = new QThread{};
    worker* w = new worker{this};
    connect(w, SIGNAL(error(clang::location, QString)), this, SLOT(error_slot(clang::location, QString)));
    connect(w, SIGNAL(indexing_uri(KUrl)), this, SLOT(indexing_uri_slot(KUrl)));
    connect(w, SIGNAL(finished()), this, SLOT(finished_slot()));

    connect(w, SIGNAL(finished()), t, SLOT(quit()));
    connect(w, SIGNAL(finished()), w, SLOT(deleteLater()));

    connect(t, SIGNAL(started()), w, SLOT(process()));
    connect(t, SIGNAL(finished()), t, SLOT(deleteLater()));
    w->moveToThread(t);
    t->start();
}

void indexer::stop()
{
}

void indexer::error_slot(clang::location loc, QString str)
{
    Q_EMIT(error(loc, str));
}

void indexer::finished_slot()
{
    kDebug() << "FINISHED: FORWARDING!";
    Q_EMIT(finished());
}

void indexer::indexing_uri_slot(KUrl file)
{
    Q_EMIT(indexing_uri(file));
}

}}                                                          // namespace index, kate
