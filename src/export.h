#ifndef _EXPORT_
#define _EXPORT_

#include "ui_export.h"

class Animation;
class Project;
class QPixmap;

class Export : public QDialog, private Ui::ExportDialog {
	Q_OBJECT;
	public:
	Export(QWidget* parent=0);
	void setProject(Project* p) { m_project=p; }

	void setAnimations(const QList<Animation*>& animations);
	
	QRect getBounds( const QList<Animation*>& list ) const;
	QPixmap* buildSpriteSheet( const QList<Animation*>& list, bool perLine);

	public slots:
	void save();
	void cancel();
	void exportFrame(Animation* anim, int frame);

	signals:
	void refreshCache(Animation* anim, bool override);


	protected:
	Project* m_project;

};

#endif

