/**
 * \file
 *
 * \brief Class \c kate::clang::compiler_options (implementation)
 *
 * \date Fri Oct 25 05:11:29 MSK 2013 -- Initial design
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

// Project specific includes
#include <src/clang/compiler_options.h>

// Standard includes
#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <iterator>
#include <ostream>

namespace kate { namespace clang {

compiler_options::compiler_options(const QStringList& options)
{
    m_shadow.resize(options.size());
    for (auto i = 0; i < options.size(); ++i)
        m_shadow[i] = options.at(i).toUtf8();
}

std::vector<const char*> compiler_options::get() const
{
    std::vector<const char*> pointers{m_shadow.size(), nullptr};
    for (auto i = 0u; i < m_shadow.size(); ++i)
        pointers[i] = m_shadow[i].constData();
    return pointers;
}

compiler_options& compiler_options::operator<<(const QString& option)
{
    m_shadow.emplace_back(option.toUtf8());
    return *this;
}

/// Make \c location printable to Qt debug streams
QDebug operator<<(QDebug dbg, const compiler_options& opts)
{
    QString result;
    for (const auto& o : opts.m_shadow)
        result += o;
    dbg.space() << result;
    return dbg.space();
}

/// Make \c location printable to C++ streams
std::ostream& operator<<(std::ostream& os, const compiler_options& opts)
{
    std::string result;
    for (const auto& o : opts.m_shadow)
        result += o.constData();
    os << result;
    return os;
}

}}                                                          // namespace clang, kate
