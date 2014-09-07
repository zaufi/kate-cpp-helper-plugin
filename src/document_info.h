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
#include <src/utils.h>

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

    /// Holds info about location of particular \c #include directive
    struct IncludeLocationData
    {
        static const unsigned ROOT = -1;
        unsigned m_header_id;                               ///< ID of the \c #included file
        unsigned m_included_by_id;                          ///< ID of a parent of the current file
        int m_line;                                         ///< inclusion line
        int m_column;                                       ///< inclusion column
    };

    explicit DocumentInfo(CppHelperPlugin*, KTextEditor::Document*);
    virtual ~DocumentInfo();

    /// Search for \c #include directives
    void scanForHeadersIncluded(const KTextEditor::Range& = KTextEditor::Range::invalid());

    /// Search collected moving ranges if there any w/ the
    /// same list as a given range
    auto isRangeWithSameLineExists(const KTextEditor::Range&) const;
    auto getPossibleProblemText(int) const;                 ///< Get an \c #include line status

    /// \name Used by #include explorer
    //@{
    void addInclusionEntry(const IncludeLocationData&);
    void clearInclusionTree();
    std::vector<unsigned> getListOfIncludedBy(unsigned) const;
    std::vector<IncludeLocationData> getListOfIncludedBy2(unsigned) const;
    std::vector<unsigned> getIncludedHeaders(unsigned) const;
    //@}

public Q_SLOTS:
    void updateStatus();

private:
    struct State
    {
        std::unique_ptr<KTextEditor::MovingRange> range;
        QString description;                                ///< Problem description if statis is not OK
        IncludeStyle type;                                  ///< \c #include type: local or global
        Status status;                                      ///< Status to use for line hint

        State(
            KTextEditor::MovingRange*
          , KTextEditor::MovingRangeFeedback*
          , IncludeStyle
          );
        State(State&&);                                     ///< Move ctor
        State& operator=(State&&);                          ///< Move-assign operator
        State(const State&) = delete;                       ///< Delete copy ctor
        State& operator=(const State&) = delete;            ///< Delete copy-assign operator
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
                  , unsigned
                  , &IncludeLocationData::m_header_id
                  >
              >
          , boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<included_by_idx>
              , boost::multi_index::member<
                    IncludeLocationData
                  , unsigned
                  , &IncludeLocationData::m_included_by_id
                  >
              >
          >
      > inclusion_index_type;

    /// Register range w/ given \c #include style (i.e. local/global)
    void addRange(KTextEditor::MovingRange*, IncludeStyle);
    void updateStatus(State&);                              ///< Recheck \c #include file status
    registered_ranges_type::iterator findRange(KTextEditor::MovingRange*);
    //BEGIN MovingRangeFeedback interface
    void caretExitedRange(KTextEditor::MovingRange*, KTextEditor::View*);
    void rangeEmpty(KTextEditor::MovingRange*);
    void rangeInvalid(KTextEditor::MovingRange*);
    //END MovingRangeFeedback interface

    CppHelperPlugin* m_plugin;                              ///< Parent plugin
    KTextEditor::Document* m_doc;                           ///< Associted document
    /// List of ranges w/ \c #incldue directives whithing a document
    registered_ranges_type m_ranges;
    inclusion_index_type m_includes;
};

inline DocumentInfo::State::State(
    KTextEditor::MovingRange* const mvr
  , KTextEditor::MovingRangeFeedback* const fbimpl
  , const IncludeStyle includedAs
  )
  : range(mvr)
  , type(includedAs)
  , status(Status::Dunno)
{
    // Subscribe to range invalidate
    range->setFeedback(fbimpl);
}

inline DocumentInfo::State::State(State&& other)
  : range(std::move(other.range))
  , type(other.type)
  , status(other.status)
{
    description.swap(other.description);
}

inline auto DocumentInfo::State::operator=(State&& other) -> State&
{
    assert("Sanity check" && this != &other);
    range = std::move(other.range);
    type = other.type;
    status = other.status;
    description.swap(other.description);
    return *this;
}

inline auto DocumentInfo::isRangeWithSameLineExists(const KTextEditor::Range& range) const
{
    for (const auto& state : m_ranges)
        if (state.range->start().line() == range.start().line())
            return true;
    return false;
}

inline auto DocumentInfo::getPossibleProblemText(const int line) const
{
    for (const auto& state : m_ranges)
        if (state.range->start().line() == line)
            return state.description;
    return QString{};
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
// kate: hl C++/Qt4;
