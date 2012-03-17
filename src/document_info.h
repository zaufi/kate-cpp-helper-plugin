/**
 * \file
 *
 * \brief Class \c kate::DocumentInfo (interface)
 *
 * \date Sun Feb 12 06:05:45 MSK 2012 -- Initial design
 */
/*
 * KateIncludeHelperPlugin is free software: you can redistribute it and/or modify it
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

#ifndef __SRC__DOCUMENTINFO_H__
#  define __SRC__DOCUMENTINFO_H__

// Project specific includes

// Standard includes
#  include <KTextEditor/MovingRange>
#  include <ktexteditor/movingrangefeedback.h>
#  include <cassert>

namespace kate {
class IncludeHelperPlugin;                                  // forward declaration

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
    enum Status
    {
        Dunno
      , NotFound
      , Ok
      , MultipleMatches
    };

    explicit DocumentInfo(IncludeHelperPlugin*);
    ~DocumentInfo();

    bool isRangeWithSameExists(const KTextEditor::Range&) const;

public Q_SLOTS:
    void addRange(KTextEditor::MovingRange*);
    void updateStatus();

private:
    struct State
    {
        KTextEditor::MovingRange* m_range;
        Status m_status;
    };

    typedef QList<State> registered_ranges_type;

    void updateStatus(State&);                              ///< Update single range
    registered_ranges_type::iterator findRange(KTextEditor::MovingRange*);
    //BEGIN MovingRangeFeedback interface
    void caretExitedRange(KTextEditor::MovingRange*, KTextEditor::View*);
    void rangeEmpty(KTextEditor::MovingRange*);
    void rangeInvalid(KTextEditor::MovingRange*);
    //END MovingRangeFeedback interface

    IncludeHelperPlugin* m_plugin;                          ///< Parent plugin
    ///< List of ranges w/ \c #incldue directives whithing a document
    registered_ranges_type m_ranges;
};

}                                                           // namespace kate
#endif                                                      // __SRC__DOCUMENTINFO_H__
