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
#include <KTextEditor/MarkInterface>
#include <KDebug>

namespace kate {

DocumentInfo::DocumentInfo(IncludeHelperPlugin* p)
  : m_plugin(p)
{
    // Subscribe slef to configuration changes
    connect(m_plugin, SIGNAL(sessionDirsChanged()), this, SLOT(updateStatus()));
    connect(m_plugin, SIGNAL(systemDirsChanged()), this, SLOT(updateStatus()));
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

void DocumentInfo::updateStatus(State& s)
{
    if (!s.m_range->isEmpty())
    {
        KTextEditor::Document* doc = s.m_range->document();
        const QString filename = doc->text(s.m_range->toRange());
        QStringList paths = findHeader(filename, m_plugin->sessionDirs(), m_plugin->globalDirs());
        if (paths.empty())
            s.m_status = NotFound;
        else if (paths.size() == 1)
            s.m_status = Ok;
        else
            s.m_status = MultipleMatches;
        kDebug() << "#include filename=" << filename << ", status=" << s.m_status;

        KTextEditor::MarkInterface* iface = qobject_cast<KTextEditor::MarkInterface*>(doc);
        const int line = s.m_range->start().line();
        switch (s.m_status)
        {
            case Ok:
                iface->removeMark(line, KTextEditor::MarkInterface::Error | KTextEditor::MarkInterface::Warning);
                break;
            case NotFound:
                iface->addMark(line, KTextEditor::MarkInterface::Error);
                iface->setMarkPixmap(KTextEditor::MarkInterface::Error, KIcon("task-reject").pixmap(QSize(16, 16)));
                break;
            case MultipleMatches:
                iface->addMark(line, KTextEditor::MarkInterface::Warning);
                iface->setMarkPixmap(KTextEditor::MarkInterface::Warning, KIcon("task-attention").pixmap(QSize(16, 16)));
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
      ) if (it->m_range == range) return it;
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

#if 1
void DocumentInfo::rangeEmpty(KTextEditor::MovingRange* range)
#else
void DocumentInfo::rangeInvalid(KTextEditor::MovingRange* range)
#endif
{
    registered_ranges_type::iterator it = findRange(range);
    if (it != m_ranges.end())
    {
        kDebug() << "MovingRange deleted: " << range;
        delete it->m_range;
        m_ranges.erase(it);
    }
}

void DocumentInfo::updateStatus()
{
}

}                                                           // namespace kate
