#ifndef INCLUDEHELPERPLUGIN_H
#define INCLUDEHELPERPLUGIN_H

#include <KTextEditor/Plugin>

namespace KTextEditor
{
	class View;
}

class IncludeHelperView;

class IncludeHelperPlugin
  : public KTextEditor::Plugin
{
  public:
    // Constructor
    explicit IncludeHelperPlugin(QObject *parent = 0, const QVariantList &args = QVariantList());
    // Destructor
    virtual ~IncludeHelperPlugin();

    void addView (KTextEditor::View *view);
    void removeView (KTextEditor::View *view);
 
    void readConfig();
    void writeConfig();
 
//     void readConfig (KConfig *);
//     void writeConfig (KConfig *);
 
  private:
    QList<class IncludeHelperView*> m_views;
};

#endif
