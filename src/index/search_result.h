/**
 * \file
 *
 * \brief Class \c kate::index::search_result (interface)
 *
 * \date Sat Nov  2 17:23:30 MSK 2013 -- Initial design
 */
/*
 * Copyright (C) 2011-2013 Alex Turbov, all rights reserved.
 * This is free software. It is licensed for use, modification and
 * redistribution under the terms of the GNU General Public License,
 * version 3 or later <http://gnu.org/licenses/gpl.html>
 *
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
#include <src/index/kind.h>

// Standard includes
#include <boost/optional.hpp>
#include <clang-c/Index.h>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <cstddef>

namespace kate { namespace index {

/**
 * \brief Structure to hold search result
 *
 * It contains everything needed for various kind of symbols.
 *
 */
struct search_result
{
    struct flags
    {
        union
        {
            unsigned m_flags_as_int;
            struct
            {
                bool m_static    : 1;                       ///< Symbol has \c static modifier
                bool m_virtual   : 1;                       ///< Symbol (function) has dynamic linkage
                bool m_const     : 1;                       ///< Symbol has \c const  modifier
                bool m_volatile  : 1;                       ///< Symbol has \c m_volatile modifier
                bool m_pod       : 1;                       ///< Symbol is a POD
                bool m_bit_field : 1;                       ///< Symbol is a bit-field
                bool m_redecl    : 1;                       ///< Is redeclaration?
                bool m_implicit  : 1;                       ///< Is implicit?
            };
        };
        flags() : m_flags_as_int{0} {}
    };

    /// \attention DO NOT FORGET TO MODIFY MOVE CTOR/ASSIGN, IF U R GOING TO ADD A NEW MEMBER.

    QString m_name;                                         ///< Symbol's name
    QString m_type;                                         ///< Symbol's type
    QString m_file;                                         ///< Filename of a declaration
    QString m_db_name;                                      ///< Name of the indexed collection of this result
    boost::optional<QStringList> m_bases;                   ///< List of base classes
    boost::optional<QString> m_scope;                       ///< Full qualified parent scope
    boost::optional<long long> m_value;                     ///< Value depended on a symbol's kind
    boost::optional<std::size_t> m_sizeof;
    boost::optional<std::size_t> m_alignof;
    boost::optional<off_t> m_offsetof;
    boost::optional<int> m_arity;
    int m_line = {-1};
    int m_column = {-1};
    kind m_kind;
    CXIdxEntityCXXTemplateKind m_template_kind = {CXIdxEntity_NonTemplate};
    CX_CXXAccessSpecifier m_access = {CX_CXXInvalidAccessSpecifier};
    flags m_flags;

    explicit search_result(const kind k)
      : m_kind{k}
    {}

    /// Delete copy ctor
    search_result(const search_result&) = delete;
    /// Delete copy-assign operator
    search_result& operator=(const search_result&) = delete;
    /// Move ctor
    search_result(search_result&&) noexcept;
    /// Move-assign operator
    search_result& operator=(search_result&&) noexcept;
};

}}                                                          // namespace index, kate
