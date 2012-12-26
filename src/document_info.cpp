/**
 * \file
 *
 * \brief Class \c kate::DocumentInfo (implementation)
 *
 * \date Sun Feb 12 06:05:45 MSK 2012 -- Initial design
 */
/*
 *  * KateCppHelperPlugin is free software: you can redistribute it and/or modify it
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
#include <src/document_info.h>
#include <src/cpp_helper_plugin.h>
#include <src/utils.h>

// Standard includes
#include <KDebug>
#include <KLocalizedString>                                 /// \todo Where is \c i18n() defiend?
#include <KTextEditor/MarkInterface>
#include <QtCore/QFileInfo>

namespace kate {

DocumentInfo::DocumentInfo(CppHelperPlugin* p)
  : m_plugin(p)
{
    // Subscribe self to configuration changes
    connect(&m_plugin->config(), SIGNAL(sessionDirsChanged()), this, SLOT(updateStatus()));
    connect(&m_plugin->config(), SIGNAL(systemDirsChanged()), this, SLOT(updateStatus()));
}

/**
 * \bug Why it crashed sometime on exit???
 */
DocumentInfo::~DocumentInfo()
{
    kDebug() << "Removing " << m_ranges.size() << " ranges...";
    for (const State& s : m_ranges)
        s.m_range->setFeedback(0);
}

void DocumentInfo::addRange(KTextEditor::MovingRange* range)
{
    assert("Sanity check" && !range->isEmpty() && range->onSingleLine());
    assert("Sanity check" && findRange(range) == m_ranges.end());
    //
    m_ranges.emplace_back(
        std::unique_ptr<KTextEditor::MovingRange>(range)
      , static_cast<KTextEditor::MovingRangeFeedback* const>(this)
      );
    // Subscribe self to range invalidate
    //
    updateStatus(m_ranges.back());
    kDebug() << "MovingRange registered: " << range;
}

void DocumentInfo::updateStatus()
{
    for (
        registered_ranges_type::iterator it = m_ranges.begin()
      , last = m_ranges.end()
      ; it != last
      ; updateStatus(*it++)
      );
}

/**
 * \todo Good idea is to check \c #include directive again and realize what kind of
 * open/close chars are used... depending on this do search for files in all configured
 * paths or session's only...
 */
void DocumentInfo::updateStatus(State& s)
{
    kDebug() << "Update status for range: " << s.m_range.get();
    if (!s.m_range->isEmpty())
    {
        KTextEditor::Document* doc = s.m_range->document();
        QString filename = doc->text(s.m_range->toRange());
        // NOTE After editing it is possible that opening '<' or '"' could
        // appear as a start symbol of the range... just try to exclude it!
        if (filename.startsWith('>') || filename.startsWith('"'))
        {
            filename.remove(0, 1);
            KTextEditor::Range shrinked = s.m_range->toRange();
            shrinked.end().setColumn(shrinked.start().column() + 1);
            s.m_range->setRange(shrinked);
        }
        // NOTE after autocompletion it is possible that closing '>' or '"' could
        // appear as the last symbol of the range... just try to exclude it!
        if (filename.endsWith('>') || filename.endsWith('"'))
        {
            filename.resize(filename.size() - 1);
            KTextEditor::Range shrinked = s.m_range->toRange();
            shrinked.end().setColumn(shrinked.end().column() - 1);
            s.m_range->setRange(shrinked);
        }
        // Reset status
        s.m_status = Status::Dunno;

        // Check if given header available
        // 0) check CWD first if allowed
        if (m_plugin->config().useCwd())
        {
            const KUrl& uri = doc->url().prettyUrl();
            const QString cur2check = uri.directory() + '/' + filename;
            kDebug() << "check current dir 4: " << cur2check;
            s.m_status = (QFileInfo(cur2check).exists()) ? Status::Ok : Status::NotFound;
        }
        // 1) Try configured dirs then
        QStringList paths = findHeader(
            filename
          , m_plugin->config().sessionDirs()
          , m_plugin->config().systemDirs()
          );
        if (paths.empty())
            s.m_status = (s.m_status == Status::Ok) ? Status::Ok : Status::NotFound;
        else if (paths.size() == 1)
            s.m_status = (s.m_status == Status::Ok) ? Status::MultipleMatches : Status::Ok;
        else
            s.m_status = Status::MultipleMatches;
        kDebug() << "#include filename=" << filename << ", status=" << int(s.m_status) << ", r=" << s.m_range.get();

        KTextEditor::MarkInterface* iface = qobject_cast<KTextEditor::MarkInterface*>(doc);
        const int line = s.m_range->start().line();
        switch (s.m_status)
        {
            case Status::Ok:
                iface->removeMark(
                    line
                  , KTextEditor::MarkInterface::Error | KTextEditor::MarkInterface::Warning
                  );
                break;
            case Status::NotFound:
                iface->removeMark(
                    line
                  , KTextEditor::MarkInterface::Error | KTextEditor::MarkInterface::Warning
                  );
                iface->setMarkPixmap(
                    KTextEditor::MarkInterface::Error
                  , KIcon("task-reject").pixmap(QSize(16, 16))
                  );
                iface->setMarkDescription(KTextEditor::MarkInterface::Error, i18n("File not found"));
                iface->addMark(line, KTextEditor::MarkInterface::Error);
                break;
            case Status::MultipleMatches:
                iface->removeMark(
                    line
                  , KTextEditor::MarkInterface::Error | KTextEditor::MarkInterface::Warning
                  );
                iface->setMarkPixmap(
                    KTextEditor::MarkInterface::Warning
                  , KIcon("task-attention").pixmap(QSize(16, 16))
                  );
                iface->setMarkDescription(KTextEditor::MarkInterface::Error, i18n("Multiple files matched"));
                iface->addMark(line, KTextEditor::MarkInterface::Warning);
                break;
            default:
                assert(!"Impossible");
        }
    }
}

/**
 * \attention Find range by given address
 */
DocumentInfo::registered_ranges_type::iterator DocumentInfo::findRange(KTextEditor::MovingRange* range)
{
    // std::find_if + lambda!
    const registered_ranges_type::iterator last = m_ranges.end();
    for (
        registered_ranges_type::iterator it = m_ranges.begin()
      ; it != last
      ; ++it
      ) if (range == it->m_range.get()) return it;
    return last;
}

void DocumentInfo::caretExitedRange(KTextEditor::MovingRange* range, KTextEditor::View*)
{
    registered_ranges_type::iterator it = findRange(range);
    if (it != m_ranges.end())
        updateStatus(*it);
}

void DocumentInfo::rangeEmpty(KTextEditor::MovingRange* range)
{
    assert(
        "Range must be valid (possible empty, but valid)"
      && range->start().line() != -1 && range->end().line() != -1
      && range->start().column() != -1 && range->end().column() != -1
      );
    // Remove possible mark on a line
    KTextEditor::MarkInterface* iface = qobject_cast<KTextEditor::MarkInterface*>(range->document());
    iface->clearMark(range->start().line());
    // Erase internal data
    registered_ranges_type::iterator it = findRange(range);
    if (it != m_ranges.end())
    {
        kDebug() << "MovingRange: empty range deleted: " << range;
        it->m_range->setFeedback(0);
        m_ranges.erase(it);
    }
}

/**
 * \note Range invalidation event may occur only if document gets relaoded
 * cuz we've made it w/ \c EmptyAllow option
 */
void DocumentInfo::rangeInvalid(KTextEditor::MovingRange* range)
{
    kDebug() << "It seems document reloaded... cleanup ranges???";
    // Erase internal data
    registered_ranges_type::iterator it = findRange(range);
    if (it != m_ranges.end())
    {
        kDebug() << "MovingRange: invalid range deleted: " << range;
        it->m_range->setFeedback(0);
        m_ranges.erase(it);
    }
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
