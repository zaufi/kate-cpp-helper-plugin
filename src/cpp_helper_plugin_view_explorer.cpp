/**
 * \file
 *
 * \brief Class \c kate::CppHelperPluginView (implementation part IV)
 *
 * Kate C++ Helper Plugin view methods related to \c #inclue explorer
 *
 * \date Sat Nov 23 16:35:42 MSK 2013 -- Initial design
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
#include "cpp_helper_plugin_view.h"
#include "cpp_helper_plugin.h"
#include "clang/to_string.h"

// Standard includes
#include <kate/mainwindow.h>
#include <KDE/KColorScheme>
#include <QtGui/QStandardItemModel>
#include <set>
#include <stack>

namespace kate { namespace details {
struct InclusionVisitorData
{
    CppHelperPluginView* const m_self;
    DocumentInfo* m_di;
    std::stack<QTreeWidgetItem*> m_parents;
    std::set<int> m_visited_ids;
    QTreeWidgetItem* m_last_added_item;
    unsigned m_last_stack_size;
};
}                                                           // namespace details

void CppHelperPluginView::updateInclusionExplorer()
{
    assert(
        "Active view supposed to be valid at this point! Am I wrong?"
      && mainWindow()->activeView()
      );

    // Show busy mouse pointer
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

    auto* doc = mainWindow()->activeView()->document();
    auto& unit = m_plugin->getTranslationUnitByDocument(doc, false);
    // Obtain diagnostic if any
    {
        auto diag = unit.getLastDiagnostic();
        m_diagnostic_data.append(
            std::make_move_iterator(begin(diag))
          , std::make_move_iterator(end(diag))
          );
    }
    details::InclusionVisitorData data = 
    {
        this
      , &m_plugin->getDocumentInfo(doc)
      , decltype(details::InclusionVisitorData::m_parents){}
      , {}
      , nullptr
      , 0
    };
    data.m_di->clearInclusionTree();                        // Clear a previous tree in the document info
    m_tool_view_interior->includesTree->clear();            // and in the tree view model
    m_includes_list_model->clear();                         // as well as `included by` list
    data.m_parents.push(m_tool_view_interior->includesTree->invisibleRootItem());
    clang_getInclusions(
        unit
      , [](
            CXFile file
          , CXSourceLocation* stack
          , unsigned stack_size
          , CXClientData d
          )
        {
            auto* const user_data = static_cast<details::InclusionVisitorData* const>(d);
            user_data->m_self->inclusionVisitor(user_data, file, stack, stack_size);
        }
      , &data
      );
    m_last_explored_document = doc;                         // Remember the document last explored

    QApplication::restoreOverrideCursor();                  // Restore mouse pointer to normal
    kDebug(DEBUG_AREA) << "headers cache now has" << m_plugin->headersCache().size() << "items!";
}

void CppHelperPluginView::inclusionVisitor(
    details::InclusionVisitorData* const data
  , CXFile file
  , CXSourceLocation* const stack
  , const unsigned stack_size
  )
{
    const auto header_name = clang::toString(clang::DCXString{clang_getFileName(file)});
    // Obtain (or assign a new) an unique header ID
    const auto header_id = m_plugin->headersCache()[header_name];

    auto included_from_id = DocumentInfo::IncludeLocationData::ROOT;
    clang::location loc;
    if (stack_size)                                         // Is there anything on stack
    {
        loc = {stack[0]};                                   // NOTE Take filename from top of stack only!
        // Obtain a filename and its ID in headers cache
        /// \todo Normalize relative paths?
        included_from_id = m_plugin->headersCache()[loc.file().toLocalFile()];
    }
    // NOTE Kate has zero based cursor position, so -1 is here
    data->m_di->addInclusionEntry({header_id, included_from_id, loc.line() - 1, loc.column() - 1});

    auto* parent = static_cast<QTreeWidgetItem*>(nullptr);
    if (data->m_last_stack_size < stack_size)               // Is current stack grew relative to the last call
    {
        assert("Sanity check!" && (stack_size - data->m_last_stack_size == 1));
        // We have to add one more parent
        data->m_parents.push(data->m_last_added_item);
        parent = data->m_last_added_item;
    }
    else if (stack_size < data->m_last_stack_size)
    {
        // Stack size reduced since tha last call: remove our top
        for (auto i = data->m_last_stack_size; i > stack_size; --i)
            data->m_parents.pop();
        assert("Stack expected to be non empty!" && !data->m_parents.empty());
        parent = data->m_parents.top();
    }
    else
    {
        assert("Sanity check!" && stack_size == data->m_last_stack_size);
        parent = data->m_parents.top();
    }
    assert("Sanity check!" && parent);
    data->m_last_added_item = new QTreeWidgetItem(parent);
    data->m_last_added_item->setText(0, header_name);
    auto it = data->m_visited_ids.find(header_id);
    if (it != end(data->m_visited_ids))
    {
        KColorScheme scheme(QPalette::Normal, KColorScheme::Selection);
        data->m_last_added_item->setForeground(
            0
          , scheme.foreground(KColorScheme::NeutralText).color()
          );
    }
    else
        data->m_visited_ids.insert(header_id);
    data->m_last_stack_size = stack_size;
}

void CppHelperPluginView::includeFileActivatedFromTree(QTreeWidgetItem* item, const int column)
{
    assert("Document expected to be alive" && m_last_explored_document);

    m_includes_list_model->clear();

    const auto& cache = const_cast<const CppHelperPlugin* const>(m_plugin)->headersCache();
    auto filename = item->data(column, Qt::DisplayRole).toString();
    auto id = cache[filename];
    if (id != HeaderFilesCache::NOT_FOUND)
    {
        const auto& di = m_plugin->getDocumentInfo(m_last_explored_document);
        auto included_from = di.getListOfIncludedBy2(id);
        for (const auto& entry : included_from)
        {
            if (entry.m_included_by_id != DocumentInfo::IncludeLocationData::ROOT)
            {
                auto* item = new QStandardItem(
                    QString("%1[%2]").arg(
                        cache[entry.m_included_by_id]
                      , QString::number(entry.m_line)
                      )
                  );
                m_includes_list_model->appendRow(item);
            }
        }
    }
    else
    {
        kDebug(DEBUG_AREA) << "WTF: " << filename << "NOT FOUND!?";
    }
}

void CppHelperPluginView::dblClickOpenFile(QString&& filename)
{
    assert("Filename expected to be non empty" && !filename.isEmpty());
    openFile(filename);
}

void CppHelperPluginView::includeFileDblClickedFromTree(QTreeWidgetItem* item, const int column)
{
    dblClickOpenFile(item->data(column, Qt::DisplayRole).toString());
}

void CppHelperPluginView::includeFileDblClickedFromList(const QModelIndex& index)
{
    assert(
        "Active view supposed to be valid at this point! Am I wrong?"
      && mainWindow()->activeView()
      );

    auto filename = m_includes_list_model->itemFromIndex(index)->text();
    const auto pos = filename.lastIndexOf('[');
    const auto line = filename.mid(pos + 1, filename.lastIndexOf(']') - pos - 1).toInt(nullptr);
    filename.remove(pos, filename.size());
    dblClickOpenFile(std::move(filename));
    mainWindow()->activeView()->setCursorPosition({line, 0});
    mainWindow()->activeView()->setSelection({line, 0, line + 1, 0});
}

}                                                           // namespace kate
