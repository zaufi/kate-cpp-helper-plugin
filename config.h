/**
 * \file
 *
 * \brief Project-wide build-time config
 *
 * \date Sun Jun 24 02:26:08 MSK 2012 -- Initial design
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


#ifndef __KATE_INCLUDE_HELPER_PLUGIN__CONFIG_H__
# define __KATE_INCLUDE_HELPER_PLUGIN__CONFIG_H__

// Project specific includes

// Standard includes
# include <QtCore/qglobal.h>
# include <kdeversion.h>

# if !defined(IHP_FORCE_BUG_WORKAROUND)
#   if (QT_VERSION == QT_VERSION_CHECK(4,8,2)) and (KDE_VERSION == KDE_MAKE_VERSION(4,8,3))
#       warning "Oops! Possible you are the victim of the bug described at cmake config time..."
#   else
#       define IHP_BROKEN_REMOVE_ITEM_WIDGET
#   endif
# else
#   define IHP_BROKEN_REMOVE_ITEM_WIDGET
# endif                                                     // !defined(FORCE_BUG_WORKAROUND)
#endif                                                      // __KATE_INCLUDE_HELPER_PLUGIN__CONFIG_H__
