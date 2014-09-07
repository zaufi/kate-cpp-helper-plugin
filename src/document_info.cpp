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
#include <KDE/KDebug>
#include <KDE/KLocalizedString>
#include <KDE/KTextEditor/MarkInterface>
#include <KDE/KTextEditor/MovingInterface>
#include <QtCore/QFileInfo>

namespace kate {
/**
 * Subscribe self to configuration changes of the plugin
 */
DocumentInfo::DocumentInfo(CppHelperPlugin* const p, KTextEditor::Document* const doc)
  : m_plugin(p)
  , m_doc(doc)
{
    connect(&m_plugin->config(), SIGNAL(sessionDirsChanged()), this, SLOT(updateStatus()));
    connect(&m_plugin->config(), SIGNAL(systemDirsChanged()), this, SLOT(updateStatus()));
}

/**
 * \bug Why it crashs sometime on exit???
 */
DocumentInfo::~DocumentInfo()
{
    kDebug(DEBUG_AREA) << "Removing " << m_ranges.size() << " ranges...";
    for (const auto& s : m_ranges)
        s.range->setFeedback(0);
}

/**
 * Search for \c #include directives
 *
 * \invariant Document supposed to have \c KTextEditor::MovingInterface implemented!
 */
void DocumentInfo::scanForHeadersIncluded(const KTextEditor::Range& source_range)
{
    auto* mv_iface = qobject_cast<KTextEditor::MovingInterface*>(m_doc);
    assert("No moving iface for document!" && mv_iface);

    auto range = (source_range == KTextEditor::Range::invalid())
      ? m_doc->documentRange()
      : source_range
      ;

    /**
     * \todo It would be \b cool to have a view class over a document
     * so the following code would be possible:
     * \code
     *  for (const auto& l : DocumentLinesView(range, doc))
     *  {
     *      QString line_str = l;    // auto converstion to strings
     *      int line_no = l.index(); // tracking of the current line number
     *  }
     *  // Or even smth like this
     *  DocumentLinesView dv = { view->document() };
     *  // Get line text by number
     *  QString line_str = dv[line_no];
     * \endcode
     */
    // Search lines and filenames #include'd in a selected range
    for (auto i = range.start().line(); i < range.end().line(); i++)
    {
        auto r = parseIncludeDirective(m_doc->line(i), false);
        if (r.range.isValid())
        {
            r.range.setBothLines(i);
            kDebug(DEBUG_AREA) << "found #include at" << r.range;
            if (!isRangeWithSameLineExists(r.range))
                addRange(
                    mv_iface->newMovingRange(
                        r.range
                      , KTextEditor::MovingRange::ExpandLeft | KTextEditor::MovingRange::ExpandRight
                      )
                  , r.type
                  );
            else kDebug(DEBUG_AREA) << "range already registered";
        }
    }
}

void DocumentInfo::addRange(KTextEditor::MovingRange* const range, const IncludeStyle style)
{
    assert("Sanity check" && !range->isEmpty() && range->onSingleLine());
    assert("Sanity check" && findRange(range) == m_ranges.end());
    //
    m_ranges.emplace_back(
        range
      , static_cast<KTextEditor::MovingRangeFeedback* const>(this)
      , style
      );
    // Find a real file and update status
    updateStatus(m_ranges.back());
    kDebug(DEBUG_AREA) << "MovingRange registered: " << range;
}

void DocumentInfo::updateStatus()
{
    std::for_each(begin(m_ranges), end(m_ranges), [this](auto& r) { this->updateStatus(r); });
}

/**
 * \todo Good idea is to check \c #include directive again and realize what kind of
 * open/close chars are used... depending on this do search for files in all configured
 * paths or session's only...
 *
 * \todo Maybe not so good, cuz this method now called from \c scanForHeadersIncluded, 
 * so parse status is fresh enough...
 */
void DocumentInfo::updateStatus(State& s)
{
    kDebug(DEBUG_AREA) << "Update status for range: " << s.range.get();
    if (!s.range->isEmpty())
    {
        auto* doc = s.range->document();
        auto filename = doc->text(s.range->toRange());
        // NOTE After editing it is possible that opening '<' or '"' could
        // appear as a start symbol of the range... just try to exclude it!
        if (filename.startsWith('>') || filename.startsWith('"'))
        {
            filename.remove(0, 1);
            auto shrinked = s.range->toRange();
            shrinked.end().setColumn(shrinked.start().column() + 1);
            s.range->setRange(shrinked);
        }
        // NOTE after autocompletion it is possible that closing '>' or '"' could
        // appear as the last symbol of the range... just try to exclude it!
        if (filename.endsWith('>') || filename.endsWith('"'))
        {
            filename.resize(filename.size() - 1);
            auto shrinked = s.range->toRange();
            shrinked.end().setColumn(shrinked.end().column() - 1);
            s.range->setRange(shrinked);
        }
        // Reset status
        s.status = Status::NotFound;

        // Check if given header available
        // 0) check CWD first if allowed or #include uses quites
        auto candidates = QStringList{};
        if (m_plugin->config().useCwd() || s.type == IncludeStyle::local)
        {
            auto uri = doc->url();
            uri.setFileName(filename);
            const auto& check = uri.path();
            kDebug(DEBUG_AREA) << "check current dir: " << check;
            if (QFileInfo{check}.exists())
            {
                s.status = Status::Ok;
                candidates << check;
            }
        }
        // 1) Try configured dirs then
        candidates << findHeader(
            filename
          , m_plugin->config().sessionDirs()
          , m_plugin->config().systemDirs()
          );
        candidates.removeDuplicates();                      // Same file could be found more than once!

        // Analyse found files and set status
        if (candidates.empty())
            s.status = Status::NotFound;
        else if (candidates.size() == 1)
            s.status = Status::Ok;
        else
            s.status = Status::MultipleMatches;
        kDebug(DEBUG_AREA) << "#include filename=" << filename <<
            ", status=" << int(s.status) <<
              ", r=" << s.range.get()
          ;

        auto* iface = qobject_cast<KTextEditor::MarkInterface*>(doc);
        const auto line = s.range->start().line();
        switch (s.status)
        {
            case Status::Ok:
                iface->removeMark(
                    line
                  , KTextEditor::MarkInterface::Error | KTextEditor::MarkInterface::Warning
                  );
                s.description.clear();
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
                iface->addMark(line, KTextEditor::MarkInterface::Error);
                s.description = i18nc("@info:tooltip", "File not found");
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
                iface->addMark(line, KTextEditor::MarkInterface::Warning);
                s.description = i18nc(
                    "@info:tooltip"
                  , "Multiple files matched: <filename>%1</filename>"
                  , candidates.join(", ")
                  );
                break;
            default:
                assert(!"Impossible");
        }
    }
}

/**
 * \attention Find range by given address!
 * Assume that documents has unique address.
 */
auto DocumentInfo::findRange(KTextEditor::MovingRange* range) -> registered_ranges_type::iterator
{
    return std::find_if(
        begin(m_ranges)
      , end(m_ranges)
      , [=](const auto& item)
        {
            return range == item.range.get();
        }
      );
}

void DocumentInfo::caretExitedRange(KTextEditor::MovingRange* range, KTextEditor::View*)
{
    auto it = findRange(range);
    if (it != end(m_ranges))
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
    auto* iface = qobject_cast<KTextEditor::MarkInterface*>(range->document());
    iface->clearMark(range->start().line());
    // Erase internal data
    auto it = findRange(range);
    if (it != m_ranges.end())
    {
        kDebug(DEBUG_AREA) << "MovingRange: empty range deleted: " << range;
        it->range->setFeedback(0);
        m_ranges.erase(it);
    }
}

/**
 * \note Range invalidation event may occur only if document gets relaoded
 * cuz we've made it w/ \c EmptyAllow option
 */
void DocumentInfo::rangeInvalid(KTextEditor::MovingRange* range)
{
    kDebug(DEBUG_AREA) << "It seems document reloaded... cleanup ranges???";
    // Erase internal data
    auto it = findRange(range);
    if (it != m_ranges.end())
    {
        kDebug(DEBUG_AREA) << "MovingRange: invalid range deleted: " << range;
        it->range->setFeedback(0);
        m_ranges.erase(it);
    }
}

std::vector<unsigned> DocumentInfo::getListOfIncludedBy(const unsigned id) const
{
    std::vector<unsigned> result;
    auto p = m_includes.get<include_idx>().equal_range(id);
    if (p.first != m_includes.get<include_idx>().end())
    {
        result.reserve(std::distance(p.first, p.second));
        std::transform(
            p.first
          , p.second
          , std::back_inserter(result)
          , [](const IncludeLocationData& item)
            {
                return item.m_included_by_id;
            }
          );
    }
#if 0
    kDebug(DEBUG_AREA) << "got" << result.size() << "items for header ID" << id;
#endif
    return result;
}

auto DocumentInfo::getListOfIncludedBy2(const unsigned id) const -> std::vector<IncludeLocationData>
{
    std::vector<IncludeLocationData> result;
    auto p = m_includes.get<include_idx>().equal_range(id);
    if (p.first != m_includes.get<include_idx>().end())
    {
        result.reserve(std::distance(p.first, p.second));
        std::copy(p.first, p.second, std::back_inserter(result));
    }
#if 0
    kDebug(DEBUG_AREA) << "got" << result.size() << "items for header ID" << id;
#endif
    return result;
}

std::vector<unsigned> DocumentInfo::getIncludedHeaders(const unsigned id) const
{
    std::vector<unsigned> result;
    auto p = m_includes.get<included_by_idx>().equal_range(id);
    if (p.first != m_includes.get<included_by_idx>().end())
    {
        result.reserve(std::distance(p.first, p.second));
        std::transform(
            p.first
          , p.second
          , std::back_inserter(result)
          , [](const IncludeLocationData& item)
            {
                return item.m_header_id;
            }
          );
    }
#if 0
    kDebug(DEBUG_AREA) << "got" << result.size() << "items for header ID" << id;
#endif
    return result;
}

}                                                           // namespace kate
// kate: hl C++/Qt4;
