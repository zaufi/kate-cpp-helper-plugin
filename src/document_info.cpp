/**
 * \file
 *
 * \brief Class \c kate::DocumentInfo (implementation)
 *
 * \date Sun Feb 12 06:05:45 MSK 2012 -- Initial design
 */
/*
 *  * KateIncludeHelperPlugin is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KateIncludeHelperPlugin is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Project specific includes
#include <src/document_info.h>
#include <src/include_helper_plugin.h>
#include <src/utils.h>

// Standard includes
#include <KDebug>
#include <KLocalizedString>                                 /// \todo Where is \c i18n() defiend?
#include <KTextEditor/MarkInterface>

namespace kate {

DocumentInfo::DocumentInfo(IncludeHelperPlugin* p)
  : m_plugin(p)
{
    // Subscribe slef to configuration changes
    connect(m_plugin, SIGNAL(sessionDirsChanged()), this, SLOT(updateStatus()));
    connect(m_plugin, SIGNAL(systemDirsChanged()), this, SLOT(updateStatus()));
}

DocumentInfo::~DocumentInfo()
{
    Q_FOREACH(const State& s, m_ranges)
    {
        kDebug() << "cleanup range: " << s.m_range;
        delete s.m_range;
    }
}

void DocumentInfo::addRange(KTextEditor::MovingRange* r)
{
    assert("Sanity check" && r->onSingleLine());
    //
    State s;
    s.m_range = r;
    s.m_range->setFeedback(this);
    s.m_status = Dunno;
    m_ranges.push_back(s);
    // Subscribe self to range invalidate
    //
    updateStatus(m_ranges.back());
    kDebug() << "MovingRange registered: " << r;
}

/**
 * \todo Good idea is to check \c #include directive again and realize what kind of
 * open/close chars are used... depending on this do search for files in all configured
 * paths or session's only...
 */
void DocumentInfo::updateStatus(State& s)
{
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
        s.m_status = Dunno;

        // Check if given header available
        // 0) check CWD first if allowed
        if (m_plugin->useCwd())
        {
            const KUrl& uri = doc->url().prettyUrl();
            const QString cur2check = uri.directory() + '/' + filename;
            kDebug() << "check current dir 4: " << cur2check;
            s.m_status = (QFileInfo(cur2check).exists()) ? Ok : NotFound;
        }
        // 1) Try configured dirs then
        QStringList paths = findHeader(filename, m_plugin->sessionDirs(), m_plugin->globalDirs());
        if (paths.empty())
            s.m_status = (s.m_status == Ok) ? Ok : NotFound;
        else if (paths.size() == 1)
            s.m_status = (s.m_status == Ok) ? MultipleMatches : Ok;
        else
            s.m_status = MultipleMatches;
        kDebug() << "#include filename=" << filename << ", status=" << s.m_status << ", r=" << s.m_range;

        KTextEditor::MarkInterface* iface = qobject_cast<KTextEditor::MarkInterface*>(doc);
        const int line = s.m_range->start().line();
        switch (s.m_status)
        {
            case Ok:
                iface->removeMark(line, KTextEditor::MarkInterface::Error | KTextEditor::MarkInterface::Warning);
                break;
            case NotFound:
                iface->removeMark(line, KTextEditor::MarkInterface::Error | KTextEditor::MarkInterface::Warning);
                iface->setMarkPixmap(KTextEditor::MarkInterface::Error, KIcon("task-reject").pixmap(QSize(16, 16)));
                iface->setMarkDescription(KTextEditor::MarkInterface::Error, i18n("File not found"));
                iface->addMark(line, KTextEditor::MarkInterface::Error);
                break;
            case MultipleMatches:
                iface->removeMark(line, KTextEditor::MarkInterface::Error | KTextEditor::MarkInterface::Warning);
                iface->setMarkPixmap(KTextEditor::MarkInterface::Warning, KIcon("task-attention").pixmap(QSize(16, 16)));
                iface->setMarkDescription(KTextEditor::MarkInterface::Error, i18n("Multiple files matched"));
                iface->addMark(line, KTextEditor::MarkInterface::Warning);
                break;
            default:
                assert(!"Impossible");
        }
    }
}

DocumentInfo::registered_ranges_type::iterator DocumentInfo::findRange(KTextEditor::MovingRange* range)
{
    // std::find_if + lambda!
    const registered_ranges_type::iterator last = m_ranges.end();
    for (
        registered_ranges_type::iterator it = m_ranges.begin()
      ; it != last
      ; ++it
      ) if (it->m_range->toRange() == range->toRange()) return it;
    return last;
}

void DocumentInfo::caretExitedRange(KTextEditor::MovingRange* range, KTextEditor::View*)
{
    registered_ranges_type::iterator it = findRange(range);
    if (it != m_ranges.end())
    {
        updateStatus(*it);
    }
}

void DocumentInfo::rangeEmpty(KTextEditor::MovingRange* range)
{
    registered_ranges_type::iterator it = findRange(range);
    if (it != m_ranges.end())
    {
        kDebug() << "MovingRange deleted: " << range;
        delete it->m_range;
        m_ranges.erase(it);
    }
}

/**
 * \note Range invalidation event may occur only if document gets relaoded
 * cuz we've made it w/ \c EmptyAllow option
 */
void DocumentInfo::rangeInvalid(KTextEditor::MovingRange* range)
{
    kDebug() << "It seems document reloaded... cleanup range...";
    rangeEmpty(range);
}

bool DocumentInfo::isRangeWithSameExists(const KTextEditor::Range& range) const
{
    // std::find_if + lambda!
    const registered_ranges_type::const_iterator last = m_ranges.end();
    for (
        registered_ranges_type::const_iterator it = m_ranges.begin()
      ; it != last
      ; ++it
      ) if (it->m_range->toRange().start().line() == range.start().line()) return true;
    return false;
}

}                                                           // namespace kate
