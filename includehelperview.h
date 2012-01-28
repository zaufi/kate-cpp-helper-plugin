#ifndef INCLUDEHELPERVIEW_H
#define INCLUDEHELPERVIEW_H

#include <QObject>
#include <KXMLGUIClient>

class IncludeHelperView : public QObject, public KXMLGUIClient
{
	Q_OBJECT
	public:
		explicit IncludeHelperView(KTextEditor::View *view = 0);
		~IncludeHelperView();
	private slots:
		void insertIncludeHelper();
	private:
		KTextEditor::View *m_view;
};

#endif
