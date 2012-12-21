/**
 * \file
 *
 * \brief Utility functions to sanitize C++ snippets
 *
 * \date Wed Dec 19 23:17:53 MSK 2012 -- Initial design
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

#ifndef __SRC__SANITIZE_SNIPPET_H__
# define __SRC__SANITIZE_SNIPPET_H__

// Project specific includes

// Standard includes
# include <QtCore/QString>

namespace kate {
/// Cleanup completion item result type
QString sanitizePrefix(QString&&);
/// Cleanup completion items parameters
QString sanitizeParams(QString&&);
/// Cleanup placeholder from completion item arguments list
QString sanitizePlaceholder(QString&&);
}                                                           // kate
#endif                                                      // __SRC__SANITIZE_SNIPPET_H__
// kate: hl C++11/Qt4;
