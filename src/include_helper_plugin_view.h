/**
 * \file
 *
 * \brief Class \c kate::IncludeHelperPluginView (interface)
 *
 * \date Mon Feb  6 06:17:32 MSK 2012 -- Initial design
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

#ifndef __SRC__INCLUDE_HELPER_PLUGIN_VIEW_H__
# define __SRC__INCLUDE_HELPER_PLUGIN_VIEW_H__

// Project specific includes

// Standard includes
# include <kate/plugin.h>
# include <KTextEditor/View>
# include <KAction>
# include <KDialog>
# include <KListWidget>
# include <KTextEdit>
# include <memory>

namespace kate {
class IncludeHelperPlugin;                                  // forward declaration

/**
 * \brief [Type brief class description here]
 *
 * [More detailed description here]
 *
 * \todo Add some suffix to slots
 *
 */
class IncludeHelperPluginView
  : public Kate::PluginView
  , public Kate::XMLGUIClient
{
    Q_OBJECT

public:
    IncludeHelperPluginView(Kate::MainWindow*, const KComponentData&, IncludeHelperPlugin*);
    virtual ~IncludeHelperPluginView();

    /// \name PluginView interface implementation
    //@{
    void readSessionConfig(KConfigBase*, const QString&);
    void writeSessionConfig(KConfigBase*, const QString&);
    //@}

private Q_SLOTS:
    void openHeader();                                      ///< Open header file under cursor
    void switchIfaceImpl();                                 ///< Open corresponding header/implementation file
    void copyInclude();                                     ///< From #include directive w/ current file in the clipboard
    void viewChanged();
    void viewCreated(KTextEditor::View*);
#if 0
    void textChanged(
        KTextEditor::Document*
      , const KTextEditor::Range&
      , const QString&
      , const KTextEditor::Range&
      );
#endif

private:
    /// Try to find file(s) w/ a given name+path and a list of possible extensions
    QStringList findCandidatesAt(const QString&, const QString&, const QStringList&);
    bool eventFilter(QObject*, QEvent*);

    KTextEditor::Range currentWord() const;                 ///< Get word under cursor as range
    void openFile(const QString&);                          ///< Open a single document
    void openFiles(const QStringList&);                     ///< Open documents for all URIs in a given list
    QStringList findFileLocations(const QString&);          ///< Get list of absolute paths to filename

    IncludeHelperPlugin* m_plugin;                          ///< Parent plugin
    KAction* m_open_header;                                 ///< <em>Open header</em> action
    KAction* m_copy_include;                                ///< <em>Copy #include to clipboard</em> action
    KAction* m_switch;                                      ///< <em>Open implementation/header</em> action
    std::unique_ptr<QWidget> m_tool_view;                   ///< Toolview to display clang diagnostic
    KTextEdit* m_diagnostic_text;                           ///< A widget to display diagnostic text
};

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
#endif                                                      // __SRC__INCLUDE_HELPER_PLUGIN_VIEW_H__
// kate: hl C++11/Qt4;
