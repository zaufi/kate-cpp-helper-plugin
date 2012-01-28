#include "includehelperplugin.h"
#include "includehelperview.h"

#include <KTextEditor/Document>
#include <KTextEditor/View>

#include <KPluginFactory>
#include <KPluginLoader>
#include <KLocale>
#include <KAction>
#include <KActionCollection>

K_PLUGIN_FACTORY(IncludeHelperPluginFactory, registerPlugin<IncludeHelperPlugin>("ktexteditor_includehelper");)
K_EXPORT_PLUGIN(IncludeHelperPluginFactory("ktexteditor_includehelper", "ktexteditor_plugins"))

IncludeHelperPlugin::IncludeHelperPlugin(QObject *parent, const QVariantList &args)
: KTextEditor::Plugin(parent)
{
	Q_UNUSED(args);
}

IncludeHelperPlugin::~IncludeHelperPlugin()
{
}

void IncludeHelperPlugin::addView(KTextEditor::View *view)
{
	IncludeHelperView *nview = new IncludeHelperView(view);
	m_views.append(nview);
}

void IncludeHelperPlugin::removeView(KTextEditor::View *view)
{
	for(int z = 0; z < m_views.size(); z++)
	{
		if(m_views.at(z)->parentClient() == view)
		{
			IncludeHelperView *nview = m_views.at(z);
			m_views.removeAll(nview);
			delete nview;
		}
	}
}

void IncludeHelperPlugin::readConfig()
{
}

void IncludeHelperPlugin::writeConfig()
{
}

IncludeHelperView::IncludeHelperView(KTextEditor::View *view)
: QObject(view)
, KXMLGUIClient(view)
, m_view(view)
{
	setComponentData(IncludeHelperPluginFactory::componentData());
	
	KAction *action = new KAction(i18n("KTextEditor - IncludeHelper"), this);
	actionCollection()->addAction("tools_includehelper", action);
	//action->setShortcut(Qt::CTRL + Qt::Key_XYZ);
	connect(action, SIGNAL(triggered()), this, SLOT(insertIncludeHelper()));
	
	setXMLFile("includehelperui.rc");
}

IncludeHelperView::~IncludeHelperView()
{
}

void IncludeHelperView::insertIncludeHelper()
{
	m_view->document()->insertText(m_view->cursorPosition(), i18n("Hello, World!"));
}

#include "includehelperview.moc"
