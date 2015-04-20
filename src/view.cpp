#include "view.h"
#include <cstdio>

#include <math.h>

#include <QGraphicsRectItem>

#include "project.h"
#include "part.h"
#include "animation.h"

#include "editcommands.h"

#define PI 3.14159265359
#define ZOOM 0.02 // Mouse zoom sensitivity


View::View(QWidget* parent) : QGraphicsView(parent), m_edit(0), m_selected(0), m_animation(0), m_frame(0) {
	m_lastOnion = 0;
	m_frameChanged = false;
	m_mode = 0;
}

void View::createWidgets() {
	m_parentLine = new QGraphicsLineItem();
	m_outline = new QGraphicsPathItem();
	m_outline->setZValue(4);
	m_parentLine->setZValue(4);
	scene()->addItem(m_parentLine);
	scene()->addItem(m_outline);

	//Create pivot widget
	m_pivot = scene()->addEllipse(-10,-10,20,20, QPen( QColor(0,0,0), 2.5f));
	m_pivotLine[0] = new QGraphicsLineItem(-16,0,16,0);
	m_pivotLine[1] = new QGraphicsLineItem(0,-16,0,16);
	m_pivotLine[0]->setParentItem(m_pivot);
	m_pivotLine[1]->setParentItem(m_pivot);
	m_pivot->setZValue(4);
	m_pivot->setFlag(QGraphicsItem::ItemIgnoresTransformations);

	selectItem(0);
}

void View::setCommandStack(CommandStack* stack) {
	m_commands = stack;
}
void View::pushCommand(Command* cmd, bool execute) {
	m_commands->push(cmd, execute);
}

void View::setMode(bool edit) {
	QPen out, ring, cross;
	if(edit) {
		QColor red(255,0,0);
		out.setColor( red );
		ring.setColor( red );
		cross.setColor( red );
	}
	out.setWidthF(2.0f);
	out.setCosmetic(true);
	ring.setWidth(2.5f);
	m_outline->setPen(out);
	m_pivot->setPen(ring);
	m_pivotLine[0]->setPen(cross);
	m_pivotLine[1]->setPen(cross);
	m_edit = edit;
}

void View::generateCache(Animation* anim, bool override) {
	if(anim) for(int i=0; i<anim->frameCount(); i++) {
		if(override || !anim->hasCache(i)) cacheFrame(anim, i);
	}
}

void View::displayFrame(Animation* anim, int frame, int before, int after) {
	//Update onion skin for previous frame if it was altered
	if(m_animation && m_frameChanged && (m_animation!=anim || m_frame!=frame)) {
		cacheFrame(m_animation, m_frame);
	}

	// Setup onion skin
	generateCache(anim, false);

	//Reposition all parts
	if(anim) updateAll(anim, frame);
	m_animation = anim;
	m_frame = frame;
	m_frameChanged = false;

	//Display onionskin
	setOnionSkin(before, after);
}

void View::updateAll(Animation* anim, int frame) {
	//Reposition all parts
	QList<int> parts = anim->parts();
	for(QList<int>::Iterator i=parts.begin(); i!=parts.end(); i++) {
		Part* part = m_project->getPart(*i);
		if(part) {
			Frame data = anim->frameData(frame, part);
			updatePart(part, data);
		}
	}
	//Update any hidden parts NOT in the list
	QList<Part*> all = m_project->parts();
	for(int i=0; i<all.size(); i++) {
		if(!parts.contains(all[i]->getID())) {
			updatePart(all[i], all[i]->hidden()? Animation::nullFrameHidden: Animation::nullFrame);
		}
	}
}


void View::updatePart(Part* part, const Frame& data) {
	//Rotation
	float ang = data.angle + (part->getParent()? part->getParent()->rotation(): 0);
	float delta = (ang - part->rotation()) * PI/180;
	part->setRotation( ang );
	rotateChildren(part, part->pos(), delta);
	//Offset
	QPointF pos = part->rest() + data.offset;
	if(part->getParent()) pos = part->getParent()->mapToItem(0, pos);
	QPointF deltaPos = pos - part->pos();
	part->setPos(pos);
	moveChildren(part, deltaPos);
	// visibility
	part->setVisible( data.visible );
	updateSelection();
	m_frameChanged = true;
}

void View::selectItem(Part* item) {
	m_selected = item;
	updateSelection();
	m_outline->setVisible(m_selected);
	m_parentLine->setVisible(m_selected && m_selected->getParent());
	m_pivot->setVisible(m_selected);
}
void View::updateSelection() {
	if(m_selected) {
		m_outline->setPath(m_selected->shape());
		m_outline->setPos(m_selected->pos());
		m_outline->setRotation(m_selected->rotation());
		m_pivot->setPos( m_selected->pos() );
		//Line to parent
		Part* parent = m_selected->getParent();
		m_parentLine->setVisible( parent );
		if(parent) {
			m_parentLine->setLine(m_selected->x(), m_selected->y(), parent->x(), parent->y());
		}
	}
}

inline QPointF View::toParent(Part* part, const QPointF& p) const {
	return part->getParent()? part->getParent()->mapFromItem(0, p): p;
}

void View::setPivot(Part* part, const QPointF& point) {
	// Calculate pivot and rest position
	QPointF offset = part->offset() - part->mapFromItem(0,point);
	QPointF rest = toParent(part, point);
	if(part->pixmap().isNull()) offset = part->offset(); // If no image, dont change offset

	//Also need to update the rest positions of the parts direct children
	if(part->children().size()) {
		QPointF delta = rest - part->rest();
		CommandGroup* group = new CommandGroup("change pivot", true);
		group->push( new ChangeRest( part, offset, rest, part->hidden()) );
		for(int i=0; i<part->children().size(); i++) {
			Part* child = part->children().at(i);
			group->push( new ChangeRest( child, child->rest() - delta, 2) );
		}
		pushCommand( group );
	} else {
		//Simply update this part
		pushCommand( new ChangeRest( part, offset, rest, part->hidden()) );
	}
}
void View::moveRest(Part* part, const QPointF& move) {
	// Translate to parent coordinates
	QPointF rest = part->rest() + toParent(part, move) - toParent(part, QPointF());
	pushCommand( new ChangeRest( part, rest, 2 ) );
}
void View::rotatePart(Part* part, float angle) {
	float oldAngle = part->localAngle();
	float newAngle = oldAngle + angle*180/PI;
	pushCommand( new ChangeFrameData( m_animation, part, m_frame, oldAngle, newAngle) );
	//Autokey
	if(m_autoKey && m_animation) {
		int key = m_animation->isKeyframe(m_frame, part);
		if(~key&1) pushCommand( new ChangeFrameData( m_animation, part, m_frame, key, key|1));
	} 
}
void View::movePart(Part* part, const QPointF& move) {
	QPointF oldPos = part->localOffset();
	QPointF newPos = oldPos + toParent(part, move) - toParent(part, QPointF());
	pushCommand( new ChangeFrameData(m_animation, part, m_frame, oldPos, newPos));
	//Autokey
	if(m_autoKey && m_animation) {
		int key = m_animation->isKeyframe(m_frame, part);
		if(~key&2) pushCommand( new ChangeFrameData( m_animation, part, m_frame, key, key|2));
	} 
}
void View::rotateChildren(Part* part, const QPointF& pivot, float angle) {
	for(QList<Part*>::Iterator i=part->children().begin(); i!=part->children().end(); ++i) {
		QPointF vp = (*i)->pos() - pivot;
		float s = sin(angle);
		float c = cos(angle);
		//Rotated vector
		QPointF dp(vp.x()*c+vp.y()*-s, vp.x()*s+vp.y()*c);
		(*i)->setPos(pivot + dp);
		(*i)->setRotation( (*i)->rotation() + angle*180/PI);
		rotateChildren(*i, pivot, angle);
	}
}
void View::moveChildren(Part* part, const QPointF& offset) {
	for(QList<Part*>::Iterator i=part->children().begin(); i!=part->children().end(); ++i) {
		(*i)->setPos( (*i)->pos() + offset);
		moveChildren(*i, offset);
	}
}

void View::moveZ(int z) { //TODO Undo for this.
	if(!m_selected) return;
	QList<QGraphicsItem*> items = scene()->items();
	//Find item
	int index = items.indexOf(m_selected);
	//Move item
	for(int i=index+z; i>=0 && i<items.size(); i+=z) {
		QGraphicsItem* item = items[i];
		if(m_selected->collidesWithItem(item)) {
			if(z<0) item->stackBefore(m_selected);
			else m_selected->stackBefore(item);
			scene()->invalidate( sceneRect() );
			break;
		}
	}
}

Part* View::partAt(const QPointF& p) {
	QList<QGraphicsItem*> items = scene()->items(p);
	for(QList<QGraphicsItem*>::Iterator i=items.begin(); i!=items.end(); i++) {
		if((*i)->data(0).toInt()) return static_cast<Part*>(*i);
	}
	return 0;
}

void View::mousePressEvent(QMouseEvent* event) {
	QPointF mpos = mapToScene(event->pos());
	m_moved = false;
	//Are we over the pivot?
	#define DIST2(a,b) ((a.x()-b.x())*(a.x()-b.x()) + (a.y()-b.y())*(a.y()-b.y()))
	float pRadius = 10 / transform().m11();
	bool onPivot = DIST2(mpos, m_pivot->pos()) < pRadius*pRadius;
	if(onPivot || m_edit) {
		m_mode = m_edit^onPivot? 1: 3; // move / set Pivot
		m_moveOffset = mpos - m_pivot->pos();
	} else if(m_selected) {
		m_mode = 2; //rotate
		m_angleOffset = atan2(mpos.y()-m_pivot->pos().y(), mpos.x()-m_pivot->pos().x());
		m_angleOffset -= m_selected->rotation() * PI/180;
		if(m_angleOffset<-PI) m_angleOffset+=2*PI;
		else if(m_angleOffset>PI) m_angleOffset-=2*PI;
	}
	//Scene size
	float dist = 3.0f / transform().m11();
	if(     fabs(mpos.x()-sceneRect().left()) < dist)	m_mode |= 0x10;
	else if(fabs(mpos.x()-sceneRect().right()) < dist)	m_mode |= 0x20;
	if(     fabs(mpos.y()-sceneRect().top()) < dist)	m_mode |= 0x40;
	else if(fabs(mpos.y()-sceneRect().bottom()) < dist)	m_mode |= 0x80;
	if(m_mode) m_commands->breakChain();
}
void View::mouseMoveEvent(QMouseEvent* event) {
	QPointF mpos = mapToScene(event->pos());
	QPointF move;
	float angle;
	if(m_selected) {
		switch(m_mode) {
		case 1: //Move
			move = mpos - m_moveOffset - m_selected->pos();
			if(m_edit) moveRest( m_selected, move );
			else if(m_animation) movePart( m_selected, move );
			break;
		case 2: //rotate
			angle = atan2(mpos.y()-m_pivot->pos().y(), mpos.x()-m_pivot->pos().x()) - m_angleOffset;
			rotatePart(m_selected, angle - m_selected->rotation()*PI/180);
			break;
		case 3: //Move pivot
			m_pivot->setPos(mpos);
			if(m_selected->getParent()) m_parentLine->setLine(mpos.x(), mpos.y(), m_selected->getParent()->x(), m_selected->getParent()->y());
			break;
		}
	}
	//Scene Size
	if(m_mode&0xf0) {
		QRectF rect = scene()->sceneRect();
		if(m_mode&0x10) rect.setLeft  ( mpos.x() );
		if(m_mode&0x20) rect.setRight ( mpos.x() );
		if(m_mode&0x40) rect.setTop   ( mpos.y() );
		if(m_mode&0x80) rect.setBottom( mpos.y() );
		scene()->setSceneRect( rect );
	}
	

	m_moved = true;
}
void View::mouseReleaseEvent(QMouseEvent* event) {
	QPointF mpos = mapToScene(event->pos());
	if(m_moved && m_mode==3 && m_selected) { //Move pivot
		setPivot( m_selected, mpos);
		m_outline->setPath( m_selected->shape());
		m_outline->setPos( m_selected->pos() );
	}
	if(m_mode) m_commands->breakChain();
	m_mode = 0;
	//Selecting
	if(!m_moved) {
		Part* p = partAt(mpos);
		if(p) m_project->select( p );
		else m_project->deselect();
	}
}
void View::wheelEvent(QWheelEvent* event) {
	if(event->modifiers() & Qt::ControlModifier) {
		float s = event->delta()>0? 1-ZOOM: 1+ZOOM;
		int v = event->delta()<0? -event->delta(): event->delta();
		while(v>0) { scale(s,s); v -= 15; }
		event->accept();
	} else QGraphicsView::wheelEvent(event);
}



void View::drawBackground(QPainter* painter, const QRectF& rect) {
	painter->fillRect(rect, QColor(240,240,240));
	painter->fillRect(sceneRect(), QColor(255,255,255));
}

void View::cacheFrame(Animation* anim, int frame) {
	updateAll(anim, frame);

	// Hide additional graphics
	Part* sel = m_selected;
	selectItem(0);
	for(int i=0; i<m_onion.size(); i++) m_onion[i]->hide();

	//Generate rect
	QRectF box;
	QList<QGraphicsItem*> items = scene()->items();
	for(int i=0; i<items.size(); i++) {
		if(items[i]->isVisible() && items[i]->data(0).toInt()) {
			if(box.isNull()) box = items[i]->sceneBoundingRect();
			else box |= items[i]->sceneBoundingRect();
		}
	}

	// Create target
	QRect rect = box.toAlignedRect(); //scene()->itemsBoundingRect().toAlignedRect();
	QPixmap image(rect.width(), rect.height());
	image.fill( Qt::transparent );
	QPainter painter( &image );
	
	// Render scene
	scene()->render( &painter, QRectF(), rect );
	anim->cacheFrame(frame, &image, rect.topLeft());

	selectItem(sel);
}

QPixmap View::fadeImage(const QPixmap& src, float alpha) {
	if(src.isNull()) return src;
	if(alpha<=0) return QPixmap();
	QPixmap img = src.copy();
	QPainter p;
	p.begin(&img);
	p.setCompositionMode( QPainter::CompositionMode_DestinationIn );
	p.fillRect(img.rect(), QColor(0,0,0,alpha*255) );
	p.end();
	return img;
}

void View::setOnionSkin(int before, int after) {
	unsigned int code = 0;
	if(m_animation) code = m_frame | (m_animation->getID()<<16) | (before<<24) | (after<<28);
	if(code==m_lastOnion) return;
	m_lastOnion = code;
	if(!m_animation) before=after=0;

	//Create QGraphicsPixmapItem objects
	while(m_onion.size() < before+after) {
		QGraphicsPixmapItem* item = new QGraphicsPixmapItem();
		m_onion.push_back( item );
		item->setZValue(-1); //Behind the rest
		scene()->addItem( item );
	}

	//Create onion skin - need before + after images
	int ix = 0; 
	if(m_animation) {
		int fc = m_animation->frameCount();
		if(before+after > fc-1) {
			before = after? (fc-1)/2: fc-1;
			after = before? (fc-1)/2: fc-1;
		}
		bool loop = m_animation->loop();
		for(int i=0; i<before+after; i++) {
			int d = i<before? i+1: i-before+1;
			int f = m_frame + (i<before? -d: d);
			if(loop && f<0) f+=fc;
			else if(loop && f>=fc) f-=fc;
			if(f>=0 && f<fc) {
				// Graphics item
				float alpha = 0.6 - d*0.2;
				QGraphicsPixmapItem* item = m_onion[ix++];
				item->setPixmap( fadeImage( m_animation->getCachedImage(f).image, alpha ) );
				item->setPos( m_animation->getCachedImage(f).point );
				item->show();
			}
		}
	}
	//hide the rest
	for(int i=ix; i<m_onion.size(); i++) m_onion[i]->hide();
}

