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

#ifndef __SRC__DOCUMENTINFO_H__
# define __SRC__DOCUMENTINFO_H__

// Project specific includes

// Standard includes
# include <KTextEditor/MovingRange>
# include <ktexteditor/movingrangefeedback.h>
# include <cassert>
# include <memory>
# include <vector>

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

    explicit DocumentInfo(CppHelperPlugin*);
    virtual ~DocumentInfo();

    bool isRangeWithSameLineExists(const KTextEditor::Range&) const;
    Status getLineStatus(const int);

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

}                                                           // namespace kate
#endif                                                      // __SRC__DOCUMENTINFO_H__
// kate: hl C++11/Qt4;
