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
#include <QtCore/QDirIterator>
#include <QtCore/QFileInfo>
#include <unistd.h>

namespace kate { namespace index {
/**
 * \class kate::index::indexer
 *
 * Scope containers:
 * - file
 * - namespace
 * - class/struct/union/enum
 * - function
 *
 * Every scope is a document.
 * Document contains a set of postings, boolean terms and values.
 * For every declaration or reference a new document will be created.
 * Possible boolean terms are:
 * - \c XDECL -- to indicate a declaration
 * - \c XREF -- to indicate a reference
 *
 * Possible values:
 * - \c REF_TO -- document ID of a declaration for a reference
 * - \c KIND -- cursor kind (function, typedef, class/struct, etc...)
 * - \c SEMANTIC_PARENT -- ID of the semantic parent document
 * - \c LEXICAL_PARENT -- ID of the lexical parent document
 * - \c LINE -- line number
 * - \c COLUMN -- column
 * - \c FILE -- filename
 */

//BEGIN worker members
worker::worker(indexer* const parent)
  : m_indexer(parent)
{
}

void worker::request_cancel()
{
    m_is_cancelled = true;
}

bool worker::is_cancelled() const
{
    return m_is_cancelled;
}

void worker::handle_file(const QString& filename)
{
    kDebug(DEBUG_AREA) << "Indexing" << filename;
}

void worker::handle_directory(const QString& directory)
{
    for (
        QDirIterator dir_it = {
            directory
          , QStringList()
          , QDir::Files | QDir::NoDotAndDotDot | QDir::Readable | QDir::CaseSensitive
          , QDirIterator::Subdirectories | QDirIterator::FollowSymlinks
          }
      ; dir_it.hasNext()
      ;
      )
    {
        kDebug(DEBUG_AREA) << "Dir scanner: found:" <<  dir_it.filePath();
    }
}

bool worker::dispatch_target(const KUrl& url)
{
    kDebug(DEBUG_AREA) << "Indexing" << url;
    if (is_cancelled())
    {
        kDebug(DEBUG_AREA) << "Cancel requested: exiting indexer thread...";
        return true;
    }
    auto target = url.toLocalFile();
    QFileInfo fi{};
    if (fi.isDir())
        handle_directory(target);
    else if (fi.isFile() && fi.isReadable())
        handle_file(target);
    else
        kDebug(DEBUG_AREA) << target << "is not a file or directory?";
    return false;
}

void worker::process()
{
    kDebug(DEBUG_AREA) << "Indexer thread has started";
    for (const auto& target : m_indexer->m_targets)
        dispatch_target(target);
    kDebug(DEBUG_AREA) << "Indexer thread has finished";
    Q_EMIT(finished());
}
//END worker members

//BEGIN indexer members
void indexer::start()
{
    QThread* t = new QThread{};
    worker* w = new worker{this};
    connect(w, SIGNAL(error(clang::location, QString)), this, SLOT(error_slot(clang::location, QString)));
    connect(w, SIGNAL(indexing_uri(KUrl)), this, SLOT(indexing_uri_slot(KUrl)));
    connect(w, SIGNAL(finished()), this, SLOT(finished_slot()));
    connect(this, SIGNAL(stopping()), w, SLOT(request_cancel()));

    connect(w, SIGNAL(finished()), t, SLOT(quit()));
    connect(w, SIGNAL(finished()), w, SLOT(deleteLater()));

    connect(t, SIGNAL(started()), w, SLOT(process()));
    connect(t, SIGNAL(finished()), t, SLOT(deleteLater()));
    w->moveToThread(t);
    t->start();
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
    Q_EMIT(finished());
}

void indexer::indexing_uri_slot(KUrl file)
{
    Q_EMIT(indexing_uri(file));
}
//END indexer members
}}                                                          // namespace index, kate
