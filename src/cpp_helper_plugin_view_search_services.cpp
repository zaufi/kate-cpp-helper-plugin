/**
 * \file
 *
 * \brief Class \c kate::CppHelperPluginView (implementation part III)
 *
 * Kate C++ Helper Plugin view methods related to indexing/searching
 *
 * \date Sat Nov 23 15:26:18 MSK 2013 -- Initial design
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
#include <src/cpp_helper_plugin_view.h>
#include <src/cpp_helper_plugin.h>
#include <src/document_proxy.h>

// Standard includes
#include <kate/mainwindow.h>
#include <QtGui/QSortFilterProxyModel>

namespace kate { namespace {
const auto CHECK_MARK = QChar{0x14, 0x27};
const auto BALOUT_X = QChar{0x17, 0x27};
}                                                           // anonymous namespace

//BEGIN SLOTS
void CppHelperPluginView::gotoDeclarationUnderCursor()
{
    assert(
        "Active view supposed to be valid at this point! Am I wrong?"
      && mainWindow()->activeView()
      );

    KTextEditor::View* view = mainWindow()->activeView();
    if (!view || !view->cursorPosition().isValid())
        return;                                             // do nothing if no view or valid cursor

    auto query = symbolUnderCursor();
    assert(
        "Symbol under cursor expected to be Ok, otherwise action must be disabled"
      && !query.isEmpty()
      );

    query = QString{"decl:" + query + " AND NOT def:y"};
    kDebug() << "Search query: " << query;
    auto results = m_plugin->databaseManager().startSearchGetResults(query);
    if (results.size() == 1)
    {
        const auto& details = results[0];
        // NOTE Kate has zero-based positioning
        openFile(details.m_file, {details.m_line - 1, details.m_column - 1});
    }
    else
    {
        setSearchQueryAndShowIt(query);
        m_search_results_model.updateSearchResults(std::move(results));
    }
}

void CppHelperPluginView::gotoDefinitionUnderCursor()
{
    assert(
        "Active view supposed to be valid at this point! Am I wrong?"
      && mainWindow()->activeView()
      );

    KTextEditor::View* view = mainWindow()->activeView();
    if (!view || !view->cursorPosition().isValid())
        return;                                             // do nothing if no view or valid cursor

    const auto symbol = symbolUnderCursor();
    assert(
        "Symbol under cursor expected to be Ok, otherwise action must be disabled"
      && !symbol.isEmpty()
      );

    auto query = QString{"decl:" + symbol + " AND def:y"};
    kDebug() << "Search query: " << query;
    auto results = m_plugin->databaseManager().startSearchGetResults(query);
    if (results.size() == 1)
    {
        const auto& details = results[0];
        // NOTE Kate has zero-based positioning
        openFile(details.m_file, {details.m_line - 1, details.m_column - 1});
    }
    else if (!results.empty())
    {
        setSearchQueryAndShowIt(query);
        m_search_results_model.updateSearchResults(std::move(results));
    }
    else
    {
        query = QString{"decl:" + symbol};
        auto results = m_plugin->databaseManager().startSearchGetResults(query);
        if (results.size() == 1)
        {
            const auto& details = results[0];
            // NOTE Kate has zero-based positioning
            openFile(details.m_file, {details.m_line - 1, details.m_column - 1});
        }
        else
        {
            setSearchQueryAndShowIt(query);
            m_search_results_model.updateSearchResults(std::move(results));
        }
    }
}

void CppHelperPluginView::searchSymbolUnderCursor()
{
    assert(
        "Active view supposed to be valid at this point! Am I wrong?"
      && mainWindow()->activeView()
      );

    KTextEditor::View* view = mainWindow()->activeView();
    if (!view || !view->cursorPosition().isValid())
        return;                                             // do nothing if no view or valid cursor

    setSearchQueryAndShowIt(symbolUnderCursor());
    startSearchDisplayResults();
#if 0
    // view is of type KTextEditor::View*
    auto* iface = qobject_cast<KTextEditor::AnnotationViewInterface*>(view);
    if (iface)
    {
        iface->setAnnotationBorderVisible(!iface->isAnnotationBorderVisible());
    }

    QByteArray filename = view->document()->url().toLocalFile().toAscii();
    auto& unit = m_plugin->getTranslationUnitByDocument(view->document());
    CXFile file = clang_getFile(unit, filename.constData());
    CXSourceLocation loc = clang_getLocation(
        unit
      , file
      , view->cursorPosition().line() + 1
      , view->cursorPosition().column() + 1
      );
    CXCursor ctx = clang_getCursor(unit, loc);
    kDebug(DEBUG_AREA) << "Cursor: " << clang::toString(clang::kind_of(ctx));
    CXCursor spctx = clang_getCursorSemanticParent(ctx);
    kDebug(DEBUG_AREA) << "Cursor of semantic parent: " << clang::toString(clang::kind_of(spctx));
    CXCursor lpctx = clang_getCursorLexicalParent(ctx);
    kDebug(DEBUG_AREA) << "Cursor of lexical parent: " << clang::toString(clang::kind_of(lpctx));

    auto comment = clang::toString(clang::DCXString{clang_Cursor_getRawCommentText(ctx)});
    kDebug(DEBUG_AREA) << "Associated comment:" << comment;
#endif
}

void CppHelperPluginView::reindexingStarted(const QString& msg)
{
    addDiagnosticMessage(
        clang::diagnostic_message{msg, clang::diagnostic_message::type::info}
      );
    // Disable rebuild index button
    m_tool_view_interior->reindexDatabase->setEnabled(false);
    m_tool_view_interior->stopIndexer->setEnabled(true);
}

void CppHelperPluginView::reindexingFinished(const QString& msg)
{
    addDiagnosticMessage(
        clang::diagnostic_message{msg, clang::diagnostic_message::type::info}
      );
    // Enable rebuild index button
    m_tool_view_interior->reindexDatabase->setEnabled(true);
    m_tool_view_interior->stopIndexer->setEnabled(false);
}

void CppHelperPluginView::startSearchDisplayResults()
{
    auto query = m_tool_view_interior->searchQuery->text();
    kDebug() << "Search query: " << query;
    if (!query.isEmpty())
    {
        auto results = m_plugin->databaseManager().startSearchGetResults(query);
        m_search_results_model.updateSearchResults(std::move(results));
    }
}

void CppHelperPluginView::searchResultsUpdated()
{
    m_tool_view_interior->searchResults->setVisible(false);
    m_tool_view_interior->searchResults->resizeColumnToContents(0);
    m_tool_view_interior->searchResults->setVisible(true);
}

void CppHelperPluginView::searchResultActivated(const QModelIndex& index)
{
    // Make it invisible while updating...
    m_tool_view_interior->details->blockSignals(true);
    clearSearchDetails();
    //
    const auto& underlaid_index = m_search_results_sortable_model->mapToSource(index);
    const auto& details = m_search_results_model.getSearchResult(underlaid_index.row());
    const auto& location = QString{R"~(<a href="#%1">%2:%3:%4</a>)~"}.arg(
        QString::number(underlaid_index.row())
      , details.m_file
      , QString::number(details.m_line)
      , QString::number(details.m_column)
      );
    appendSearchDetailsRow(i18nc("@label", "Location:"), location, false);
    if (!details.m_type.isEmpty())
        appendSearchDetailsRow(i18nc("@label", "Type:"), details.m_type);
    if (details.m_flags.m_decl)
        appendSearchDetailsRow(i18nc("@label", "Declaration:"), CHECK_MARK, false);
    else
        appendSearchDetailsRow(i18nc("@label", "Reference:"), CHECK_MARK, false);
    if (details.m_flags.m_redecl)
        appendSearchDetailsRow(i18nc("@label", "Definition:"), CHECK_MARK, false);
    if (details.m_flags.m_implicit)
        appendSearchDetailsRow(i18nc("@label", "Implicit:"), CHECK_MARK, false);
    if (details.m_scope)
        appendSearchDetailsRow(i18nc("@label", "Scope:"), details.m_scope.get());
    else
        appendSearchDetailsRow(i18nc("@label", "Scope:"), QString{"global"});
    if (details.m_access != CX_CXXInvalidAccessSpecifier)
    {
        switch (details.m_access)
        {
            case CX_CXXPublic:
                appendSearchDetailsRow(i18nc("@label", "Visibility:"), QString{"public"});
                break;
            case CX_CXXProtected:
                appendSearchDetailsRow(i18nc("@label", "Visibility:"), QString{"protected"});
                break;
            case CX_CXXPrivate:
                appendSearchDetailsRow(i18nc("@label", "Visibility:"), QString{"private"});
                break;
            default:
                assert(!"Invalid value of CX_CXXInvalidAccessSpecifier");
                break;
        }
    }
    if (details.m_bases)
    {
        const auto bases = details.m_bases.get().join(", ");
        appendSearchDetailsRow(
            i18ncp("@label", "Base class:", "Base classes:", details.m_bases.get().size())
          , bases
          );
    }
    switch (details.m_kind)
    {
        case index::kind::ENUM_CONSTANT:
            assert("Sanity check" && details.m_value);
            appendSearchDetailsRow(i18nc("@label", "Value:"), QString::number(details.m_value.get()));
            break;
        case index::kind::BITFIELD:
            assert("Sanity check" && details.m_value);
            appendSearchDetailsRow(i18nc("@label", "Width:"), QString::number(details.m_value.get()));
            break;
        default:
            break;
    }
    if (details.m_arity)
        appendSearchDetailsRow(i18nc("@label", "Arity:"), QString::number(details.m_arity.get()), false);
    if (details.m_sizeof)
        appendSearchDetailsRow(i18nc("@label", "Size:"), QString::number(details.m_sizeof.get()), false);
    if (details.m_alignof)
        appendSearchDetailsRow(i18nc("@label", "Align:"), QString::number(details.m_alignof.get()), false);
    if (details.m_flags.m_virtual)
        appendSearchDetailsRow(i18nc("@label means C++ virtual function-member", "Virtual:"), CHECK_MARK, false);
    if (details.m_flags.m_static)
        appendSearchDetailsRow(i18nc("@label", "Static:"), CHECK_MARK, false);
    if (details.m_flags.m_const)
        appendSearchDetailsRow(i18nc("@label", "Const:"), CHECK_MARK, false);
    if (details.m_flags.m_volatile)
        appendSearchDetailsRow(i18nc("@label", "Volatile:"), CHECK_MARK, false);
    if (details.m_flags.m_pod)
        appendSearchDetailsRow(i18nc("@label", "POD:"), CHECK_MARK, false);

    appendSearchDetailsRow(i18nc("@label", "Found in:"), details.m_db_name);

    // Make it visible again
    m_tool_view_interior->details->blockSignals(false);
}

void CppHelperPluginView::locationLinkActivated(const QString& link)
{
    kDebug() << "link=" << link;
    assert("Unexpected link format" && link[0] == '#');
    const auto row = link.mid(1).toInt();
    const auto& details = m_search_results_model.getSearchResult(row);
    // NOTE Kate has zero-based positioning
    openFile(details.m_file, {details.m_line - 1, details.m_column - 1});
}
//END SLOTS

//BEGIN Utility (private) functions
/**
 * \todo Do not allow multiline selection! Really?!
 */
QString CppHelperPluginView::symbolUnderCursor()
{
    KTextEditor::View* view = mainWindow()->activeView();
    assert("Active view expected to be valid!" && view);
    if (view->selection())
    {
        return view->selectionText();
    }
    else if (view->cursorPosition().isValid())
    {
        DocumentProxy doc = mainWindow()->activeView()->document();
        auto range = doc.getIdentifierUnderCursor(view->cursorPosition());
        kDebug(DEBUG_AREA) << "current word range: " << range;
        if (range.isValid() && !range.isEmpty())
            return doc->text(range);
    }
    return QString{};
}

inline void CppHelperPluginView::appendSearchDetailsRow(
    const QString& label
  , const QString& value
  , const bool selectable
  )
{
    auto* text = new QLabel{value};
    auto sizePolicy = QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Expanding};
    text->setSizePolicy(sizePolicy);
    text->setWordWrap(true);
    if (selectable)
        text->setTextInteractionFlags(Qt::TextSelectableByMouse);
    connect(
        text
      , SIGNAL(linkActivated(const QString&))
      , this
      , SLOT(locationLinkActivated(const QString&))
      );
    m_tool_view_interior->detailsFormLayout->addRow(label, text);
}

inline void CppHelperPluginView::appendSearchDetailsRow(const QString& label, const bool flag)
{
    auto* cb = new QCheckBox{};
    cb->setChecked(flag);
    cb->setEnabled(false);
    m_tool_view_interior->detailsFormLayout->addRow(label, cb);
}

inline void CppHelperPluginView::clearSearchDetails()
{
    while (auto* item = m_tool_view_interior->detailsFormLayout->takeAt(0))
    {
        if (auto* widget = item->widget())
            delete widget;
        delete item;
    }
}

void CppHelperPluginView::setSearchQueryAndShowIt(const QString& query)
{
    m_tool_view_interior->searchQuery->setText(query);
    const auto search_tab_idx = m_tool_view_interior->tabs->indexOf(m_tool_view_interior->searchTab);
    m_tool_view_interior->tabs->setCurrentIndex(search_tab_idx);
    mainWindow()->showToolView(m_tool_view.get());
    m_tool_view_interior->searchQuery->setFocus(Qt::ActiveWindowFocusReason);
}
//END Utility (private) functions

}                                                           // namespace kate
