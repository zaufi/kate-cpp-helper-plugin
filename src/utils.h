/**
 * \file
 *
 * \brief Class \c kate::utils (interface)
 *
 * \date Mon Feb  6 04:36:38 MSK 2012 -- Initial design
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

#ifndef __SRC__UTILS_H__
#  define __SRC__UTILS_H__

// Project specific includes

// Standard includes
#  include <KTextEditor/Range>

namespace kate {
/// Get filename of \c #include directive at given line
KTextEditor::Range parseIncludeDirective(const QString&, const bool);
/// \c true if given MIME type string belongs to C/C++ source
inline bool isCOrPPSource(const QString& mime_str)
{
    return (mime_str == "text/x-c++src")
      || (mime_str == "text/x-c++hdr")
      || (mime_str == "text/x-csrc")
      || (mime_str == "text/x-chdr")
      ;
}
}                                                           // namespace kate
#endif                                                      // __SRC__UTILS_H__
