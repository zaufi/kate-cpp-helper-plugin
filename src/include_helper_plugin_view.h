/**
 * \file
 *
 * \brief Class \c kate::include_helper_plugin_view (interface)
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
#  define __SRC__INCLUDE_HELPER_PLUGIN_VIEW_H__

// Project specific includes

// Standard includes
#  include <kate/plugin.h>
#  include <KTextEditor/View>
#  include <KAction>
#  include <KDialog>
#  include <KListWidget>

namespace kate {
class IncludeHelperPlugin;                                  // forward declaration

/**
 * \brief [Type brief class description here]
 *
 * [More detailed description here]
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
    void copyInclude();                                     ///< From #include directive w/ current file in the clipboard
    void viewChanged();
    void viewCreated(KTextEditor::View*);
    void updateDocumentInfo(KTextEditor::Document*);
    void textInserted(KTextEditor::Document*, const KTextEditor::Range&);
    void textChanged(
        KTextEditor::Document*
      , const KTextEditor::Range&
      , const QString&
      , const KTextEditor::Range&
      );

private:
    KTextEditor::Range currentWord() const;                 ///< Get word under cursor as range

    IncludeHelperPlugin* m_plugin;                          ///< Parent plugin
    KAction* m_open_header;                                 ///< <em>Open header</em> action
    KAction* m_copy_include;                                ///< <em>Copy #include to clipboard</em> action
};

class ChooseFromListDialog : public KDialog
{
    Q_OBJECT

public:
    ChooseFromListDialog(QWidget*);
    static QStringList select(QWidget*, const QStringList&);

private:
    KListWidget* m_list;
};

}                                                           // namespace kate
#endif                                                      // __SRC__INCLUDE_HELPER_PLUGIN_VIEW_H__
