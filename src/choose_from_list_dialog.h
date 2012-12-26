/**
 * \file
 *
 * \brief Class \c kate::ChooseFromListDialog (interface)
 *
 * \date Tue Dec 25 05:49:02 MSK 2012 -- Initial design
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

#ifndef __SRC__CHOOSE_FROM_LIST_DIALOG_H__
# define __SRC__CHOOSE_FROM_LIST_DIALOG_H__

// Project specific includes

// Standard includes
# include <KDialog>
# include <KListWidget>

namespace kate {

/**
 * \brief [Type brief class description here]
 *
 * [More detailed description here]
 *
 */
class ChooseFromListDialog : public KDialog
{
    Q_OBJECT

public:
    ChooseFromListDialog(QWidget*);
    static QString selectHeaderToOpen(QWidget*, const QStringList&);

private:
    KListWidget* m_list;
};

}                                                           // namespace kate
#endif                                                      // __SRC__CHOOSE_FROM_LIST_DIALOG_H__
// kate: hl C++11/Qt4;
