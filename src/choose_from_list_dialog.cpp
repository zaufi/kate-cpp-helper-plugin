/**
 * \file
 *
 * \brief Class \c kate::ChooseFromListDialog (implementation)
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

// Project specific includes
#include "choose_from_list_dialog.h"

// Standard includes
#include <KDE/KListWidget>
#include <KDE/KLocalizedString>
#include <KDE/KSharedConfig>
#include <cassert>

namespace kate {
//BEGIN ChooseFromListDialog
ChooseFromListDialog::ChooseFromListDialog(QWidget* parent)
  : KDialog(parent)
{
    setModal(true);
    setButtons(KDialog::Ok | KDialog::Cancel);
    showButtonSeparator(true);
    setCaption(i18nc("@title:window", "Choose Header File to Open"));

    m_list = new KListWidget{this};
    setMainWidget(m_list);

    connect(m_list, SIGNAL(executed(QListWidgetItem*)), this, SLOT(accept()));
}

QString ChooseFromListDialog::selectHeaderToOpen(QWidget* parent, const QStringList& strings)
{
    KConfigGroup gcg(KGlobal::config(), "CppHelperChooserDialog");
    ChooseFromListDialog dialog(parent);
    dialog.m_list->addItems(strings);                       // append gien items to the list
    if (!strings.isEmpty())                                 // if strings list isn't empty
    {
        dialog.m_list->setCurrentRow(0);                    // select 1st row in the list
        dialog.m_list->setFocus(Qt::TabFocusReason);        // and give focus to it
    }
    dialog.restoreDialogSize(gcg);                          // restore dialog geometry from config

    QStringList result;
    if (dialog.exec() == QDialog::Accepted)                 // if user accept smth
    {
        // grab seleted items into a result list
        for (const QListWidgetItem* item : dialog.m_list->selectedItems())
            result.append(item->text());
    }
    dialog.saveDialogSize(gcg);                             // write dialog geometry to config
    gcg.sync();
    if (result.empty())
        return QString{};
    assert("The only file expected" && result.size() == 1u);
    return result[0];
}
//END ChooseFromListDialog
}                                                           // namespace kate
// kate: hl C++/Qt4;
