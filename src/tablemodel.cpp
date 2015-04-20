#include <QPainter>
#include "tablemodel.h"
#include "project.h"
#include "animation.h"
#include "part.h"

#include <cstdio>

TableModel::TableModel(QObject* parent) {
	m_animation = 0;
	m_mode = 0;
}

void TableModel::setAnimation(Animation* anim) {
	m_animation = anim;
	reset(); // Flag change
}

int TableModel::rowCount(const QModelIndex& parent) const {
	return 1;
}
int TableModel::columnCount(const QModelIndex& parent) const {
	return m_animation? m_animation->frameCount() : 0;
}

QVariant TableModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if(role==Qt::SizeHintRole) return QSize(1,1);
	return QVariant();
}

QVariant TableModel::data(const QModelIndex& index, int role) const {
	if(!index.isValid() || !m_animation) return QVariant();
	int frame = index.column();
	Part* part = m_project->currentPart();
	switch(role) {
	case 32: return frame - m_project->frame();			// Current frame
	case 33: return part? m_animation->isKeyframe(frame, part): 0;	// Part keyframe
	case 34: return m_animation->isKeyframe(frame);			// Any keyframe
	}
	return QVariant();
}

//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////

TableDelegate::TableDelegate(QObject* parent): QAbstractItemDelegate(parent) {
}
QSize TableDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
	return QSize(18,24);
}
void TableDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
	//Selection
	if(option.state & QStyle::State_Selected) {
		painter->fillRect(option.rect, option.palette.highlight());
	}
	//Current frame?
	if( index.model()->data(index, 32).toInt()==0 ) {
		painter->fillRect(option.rect, QBrush( QColor(255,128,0) ) );
		//painter->drawRect(option.rect);

	}

	int mode = index.model()->data(index, 33).toInt();
	int amode = mode?0: index.model()->data(index, 34).toInt();

	//circles until i do the graphics
	painter->save();
	float cx = option.rect.x() + option.rect.width()/2;
	float cy = option.rect.y() + option.rect.height()/2;
	painter->setRenderHint(QPainter::Antialiasing, true);
	if(mode) painter->setBrush(option.palette.text());
	if(mode || amode) painter->drawEllipse( QRectF(cx-3,cy-3,6,6) );
	painter->restore();
}


