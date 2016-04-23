#include "export.h"
#include "project.h"
#include "animation.h"

#include <QPainter>
#include <QFileDialog>

#include <cstdio>
#include <cmath>

Export::Export(QWidget* parent) {
	setupUi(this);
	setWindowFlags( Qt::Dialog | Qt::WindowStaysOnTopHint );
	animationList->setSortingEnabled(true);

	connect( buttons, SIGNAL( accepted() ), this, SLOT( save() ));
	connect( buttons, SIGNAL( rejected() ), this, SLOT( cancel() ));
}

void Export::setAnimations(const QList<Animation*>& animations) {
	animationList->clear();
	for(int i=0; i<animations.size(); i++) {
		QListWidgetItem* item = new QListWidgetItem( animations[i]->name() );
		item->setCheckState( Qt::Checked );
		item->setData(32, animations[i]->getID() );
		animationList->addItem( item );
	}
}

void Export::save() {
	printf("Export Animations\n");
	QString file = QFileDialog::getSaveFileName( this, "Export Animations", QString::null, "Images (*.png)");
	if(file!=QString::null) {
		//Compile animaition list
		QList<Animation*> list;
		for(int i=0; i<animationList->count(); i++) {
			QListWidgetItem* item = animationList->item(i);
			if(item->checkState() == Qt::Checked) {
				Animation* anim = m_project->getAnimation( item->data(32).toInt() );
				if(anim) list.push_back(anim);
			}
		}
		//build spritesheet
		QPixmap* image = buildSpriteSheet(list, perLine->isChecked());
		if(image) image->save(file);
	}
}

void Export::cancel() {
	printf("Cancel\n");
}

void Export::exportFrame(Animation* anim, int frame) {
	QString file = QFileDialog::getSaveFileName( this, "Export Animation", QString::null, "Images (*.png)");
	if(file!=QString::null) {
		printf("Export Frame\n");
		m_project->setCurrent(anim);
		anim->getCachedImage(frame).image.save(file);
	}
}

QRect Export::getBounds( const QList<Animation*>& list ) const {
	// Get bounding box
	QRect box;
	for(int a = 0; a<list.size(); a++) {
		Animation* anim = list[a];
		for(int i=0; i<anim->frameCount(); i++) {
			QRect r   = anim->getCachedImage(i).image.rect();
			QPoint p = anim->getCachedImage(i).point;
			r.translate(p);
			if(i==0 && a==0) box = r; else box |= r;
		}
	}
	box.adjust(0,0,box.width()&1? 1:0, box.height()&1?1:0);	// Hack: Make even
	return box;
}

QPixmap* Export::buildSpriteSheet( const QList<Animation*>& list, bool animPerRow) {
	Animation* anim = m_project->currentAnimation();
	// Get animation range
	int longest = 1, total = 0;
	for(int i = 0; i<list.size(); i++) {
		int fc = list[i]->frameCount();
		if(fc>longest) longest = fc;
		total += fc;
		// Make sure animation is fully cached
		refreshCache(list[i], true);
	}
	m_project->setCurrent(anim);

	// Get bounds
	QRect box = getBounds(list);
	if(box.width()<0 || box.height()<0) {
		printf("Error: Null image bounds\n");
		return 0;
	}

	// Output image size
	int iw, ih;
	if(animPerRow) {
		iw = longest * box.width();
		ih = list.size() * box.height();
	} else {
		//Square
		float ratio = (float) box.width() / box.height();
		iw = ceil(sqrt(total) / ratio);
		ih = ceil((float)total / iw);
		iw *= box.width();
		ih *= box.height();
	}

	printf("Export image %dx%d (%d frames of %dx%d on grid %dx%d)\n", iw,ih, total, box.width(), box.height(), iw/box.width(), ih/box.height());
	
	// Paint image
	QPoint point;
	QPoint base = box.topLeft();
	QPixmap* image = new QPixmap(iw, ih);
	image->fill( Qt::transparent );
	QPainter p;
	p.begin(image);
	for(int a=0; a<list.size(); a++) {
		for(int i=0; i<list[a]->frameCount(); i++) {
			QPoint offset = list[a]->getCachedImage(i).point;
			p.drawPixmap( point+offset-base, list[a]->getCachedImage(i).image );
			//DEBUG: Draw frame rect
			//p.drawRect(box.translated(point-base));
			point.rx() += box.width();
			if(point.x() >= iw || (animPerRow && i==list[a]->frameCount()-1)) {
				point.rx() = 0;
				point.ry() += box.height();
			}
		}
	}
	p.end();
	//Return image
	return image;
}


