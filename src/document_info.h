/**
 * \file
 *
 * \brief Class \c kate::DocumentInfo (interface)
 *
 * \date Sun Feb 12 06:05:45 MSK 2012 -- Initial design
 */
/*
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

// Standard includes
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/indexed_by.hpp>
#include <KDE/KTextEditor/MovingRange>
/// \todo Replace w/ approprite file
#include <ktexteditor/movingrangefeedback.h>
#include <cassert>
#include <memory>
#include <vector>

namespace kate {
class CppHelperPlugin;                                  // forward declaration

/**
 * \brief [Type brief class description here]
 *
 * \todo Use file monitor to watch for file changes...
 *
 */
class DocumentInfo
  : public QObject
  , private KTextEditor::MovingRangeFeedback
{
    Q_OBJECT

public:
    enum struct Status
    {
        Dunno
      , NotFound
      , Ok
      , MultipleMatches
    };

    struct IncludeLocationData
    {
        static const int ROOT = -1;
        int m_header_id;                                    ///< ID of the \c #included file
        int m_included_by_id;                               ///< ID of a parent of the current file
        int m_line;                                         ///< inclusion line
        int m_column;                                       ///< inclusion column
    };

    explicit DocumentInfo(CppHelperPlugin*);
    virtual ~DocumentInfo();

    bool isRangeWithSameLineExists(const KTextEditor::Range&) const;
    Status getLineStatus(const int);
    void addInclusionEntry(const IncludeLocationData&);
    void clearInclusionTree();
    std::vector<int> getListOfIncludedBy(const int) const;
    std::vector<IncludeLocationData> getListOfIncludedBy2(const int) const;
    std::vector<int> getIncludedHeaders(const int) const;

public Q_SLOTS:
    void addRange(KTextEditor::MovingRange*);
    void updateStatus();

private:
    struct State
    {
        std::unique_ptr<KTextEditor::MovingRange> m_range;
        Status m_status;

        State(
            std::unique_ptr<KTextEditor::MovingRange>&&
          , KTextEditor::MovingRangeFeedback*
          );
        State(State&&) = default;                           ///< Default move ctor
        State& operator=(State&&) = default;                ///< Default move-assign operator
    };

    typedef std::vector<State> registered_ranges_type;

    /// Type to hold index of header files and assigned IDs
    struct include_idx;
    struct included_by_idx;
    typedef boost::multi_index_container<
        IncludeLocationData
      , boost::multi_index::indexed_by<
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<include_idx>
              , boost::multi_index::member<
                    IncludeLocationData
                  , int
                  , &IncludeLocationData::m_header_id
                  >
              >
          , boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<included_by_idx>
              , boost::multi_index::member<
                    IncludeLocationData
                  , int
                  , &IncludeLocationData::m_included_by_id
                  >
              >
          >
      > inclusion_index_type;

    void updateStatus(State&);                              ///< Update single range
    registered_ranges_type::iterator findRange(KTextEditor::MovingRange*);
    //BEGIN MovingRangeFeedback interface
    void caretExitedRange(KTextEditor::MovingRange*, KTextEditor::View*);
    void rangeEmpty(KTextEditor::MovingRange*);
    void rangeInvalid(KTextEditor::MovingRange*);
    //END MovingRangeFeedback interface

    CppHelperPlugin* m_plugin;                              ///< Parent plugin
    ///< List of ranges w/ \c #incldue directives whithing a document
    registered_ranges_type m_ranges;
    inclusion_index_type m_includes;
};

inline DocumentInfo::State::State(
    std::unique_ptr<KTextEditor::MovingRange>&& range
  , KTextEditor::MovingRangeFeedback* fbimpl
  )
  : m_range(std::move(range))
  , m_status(Status::Dunno)
{
    m_range->setFeedback(fbimpl);
}

inline bool DocumentInfo::isRangeWithSameLineExists(const KTextEditor::Range& range) const
{
    for (const auto& state : m_ranges)
        if (state.m_range->start().line() == range.start().line())
            return true;
    return false;
}

inline auto DocumentInfo::getLineStatus(const int line) -> Status
{
    for (const auto& state : m_ranges)
        if (state.m_range->start().line() == line)
            return state.m_status;
    return Status::Dunno;
}

inline void DocumentInfo::addInclusionEntry(const IncludeLocationData& item)
{
    m_includes.insert(item);
}

inline void DocumentInfo::clearInclusionTree()
{
    m_includes.clear();
}

}                                                           // namespace kate
// kate: hl C++11/Qt4;
