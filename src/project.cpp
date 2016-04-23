#include "project.h"

#include "animation.h"
#include "part.h"
#include "ik.h"

#include <cstdio>

Project::Project(QObject* parent) : QObject(parent),
	m_partValue(1), m_animValue(1), m_controllerValue(1),
	m_currentPart(0), m_currentAnimation(0), m_currentFrame(0) {
}
Project::~Project() {
	clear();
}
QString Project::getTitle() const {
	if(m_file.isEmpty()) return "untitled";
	int l = m_file.lastIndexOf("/");
	if(l>=0) return m_file.right( m_file.size() - l - 1);
	l = m_file.lastIndexOf("\\");
	if(l>=0) return m_file.right( m_file.size() - l - 1);
	return m_file;
}

//// //// //// //// //// //// //// //// Part Functions //// //// //// //// //// //// //// ////

Part* Project::getPart(int id) {
	return m_parts.value( id, 0 );
}
Part* Project::createPart(const QString& name, int id, bool null) {
	//Assign id
	if(id) m_partValue = m_partValue>id? m_partValue: id+1;
	else id = ++m_partValue;
	//Create part
	Part* part = new Part(id, null);
	part->setName( name );
	part->setData(0, id);
	return part;
}
void Project::addPart(Part* part, Part* parent) {
	//Set parent
	QPointF rest = part->rest();
	part->setParent( parent );
	part->setRest(rest); //Dont change rest position
	//Add to map
	m_parts[ part->getID() ] = part;
	//Add to scene
	m_scene.addItem( part );
	//Flag change
	changedPart( part->getID() );
}
void Project::removePart(Part* part) {
	int id = part->getID();
	//Delete children first
	for(int i=0; i<part->children().size(); i++) removePart( part->children()[i] );
	m_scene.removeItem(part);
	m_parts.remove( id );
	if(part->getParent()) part->setParent(0);
	//Remove from selections
	m_selection.removeAll(part);
	if(m_currentPart==part) m_currentPart = 0;
	delete part;
	changedPart(id);
}

Part* Project::clonePart(Part* part, Part* parent) {
	Part* p = createPart( part->getName() + "_copy", 0, part->isNull() );
	addPart(p, parent);

	// Copy data
	p->setImage( part->pixmap() );
	p->setSource( part->getSource() );
	p->setOffset( part->offset() );
	p->setRest( part->rest() );
	p->setHidden( part->hidden() );
	p->setPos( part->pos() );
	p->setRotation( part->rotation() );

	// Copy animation
	// TODO
	
	return p;
}


//// //// //// //// //// //// //// //// Selection Functions //// //// //// //// //// //// //// ////

void Project::select(Part* part) {
	bool flag = part != m_currentPart;
	if(!m_selection.contains(part)) {
		m_selection.push_back( part );
		flag = true;
	}
	m_currentPart = part;
	if(flag) changedSelection(part);
}
void Project::select(QList<Part*> parts) {
	for(int i=0; i<parts.size(); i++) {
		if(!m_selection.contains(parts[i])) m_selection.push_back(parts[i]);
	}
	if(!m_currentPart && parts.size()) m_currentPart = parts[0];
	changedSelection((Part*)0);
}
void Project::deselect(Part* part) {
	if(m_selection.removeAll(part)) {
		if(m_currentPart==part) m_currentPart = m_selection.size()? m_selection[0]: 0;
		changedSelection(part);
	}
}
void Project::deselect() {
	m_selection.clear();
	m_currentPart = 0;
	changedSelection((Part*)0);
}
Part* Project::currentPart() {
	return m_currentPart;
}
const QList<Part*> Project::parts() {
	//Return list of parts
	return m_parts.values();
}

//// //// //// //// //// //// //// //// Animation Functions //// //// //// //// //// //// //// ////

void Project::addAnimation(Animation* anim, int id) {
	anim->setID( id?id:++m_animValue );
	m_animations.push_back(anim);
	changedAnimation(anim->getID());
}
void Project::removeAnimation(Animation* anim) {
	for(int i=0; i<m_animations.size(); i++) {
		if(m_animations[i]==anim) {
			m_animations.removeAll( anim );
			changedAnimation( anim->getID() );
			//remove from selection
			if(m_currentAnimation==anim) setCurrent(0);
			printf("Deleting Animation %p\n", anim);
			delete anim;
			break;
		}
	}
}
void Project::moveAnimation(Animation* anim, int index) {
	if(index<0) index=0;
	else if(index>=m_animations.size()) index = m_animations.size()-1;
	int old = m_animations.indexOf(anim);
	if(old!=index) {
		m_animations.removeOne(anim);
		m_animations.insert(index, anim);
		changedAnimation( anim->getID() );
	}
}
void Project::setCurrent(Animation* anim) {
	m_currentAnimation = anim;
	changedSelection(anim);
}
Animation* Project::getAnimation(int id) {
	for(int i=0; i<m_animations.size(); i++) {
		if(m_animations[i]->getID() == id) return m_animations[i];
	}
	return 0;
}
Animation* Project::currentAnimation() {
	return m_currentAnimation;
}
const QList<Animation*>& Project::animations() {
	return m_animations;
}

//// //// //// //// //// //// //// //// Controllers //// //// //// //// //// //// //// ////

const QList<IKController*>& Project::controllers() const {
	return m_controllers;
}
int Project::getControllerIndex(int id) const {
	for(int i=0; i<m_controllers.size(); ++i) {
		if(m_controllers[i]->getID() == id) return i;
	}
	return -1;
}
IKController* Project::getController(int id) const {
	foreach(IKController* c, m_controllers) {
		if(c->getID() == id) return c;
	}
	return 0;
}
int Project::setController(int id, Part* a, Part* b, Part* h, Part* g) {
	if(id==0) id = ++m_controllerValue;
	IKController* ik = new IKController(id, a,b,h,g);
	// Replace existing one
	for(int i=0; i<m_controllers.size(); ++i) {
		if(m_controllers[i]->getID() == id) {
			delete m_controllers[i];
			m_controllers[i] = ik;
			changedController(ik->getID());
			return id;
		}
	}
	// Add new
	m_controllers.push_back(ik);
	changedController(ik->getID());
	return id;
}
void Project::swapControllers(int indexA, int indexB) {
	m_controllers.swap(indexA, indexB);
	changedController( m_controllers[indexA]->getID() );
	changedController( m_controllers[indexB]->getID() );
}
void Project::removeController(int id) {
	for(int i=0; i<m_controllers.size(); ++i) {
		if(m_controllers[i]->getID() == id) {
			delete m_controllers[i];
			m_controllers.removeAt(i);
			changedController(id);
			return;
		}
	}
}

//// //// //// //// //// //// //// //// Other Functions //// //// //// //// //// //// //// ////

void Project::clear() {
	//Clear selection
	m_currentFrame = 0;
	deselect();
	setCurrent(0);

	//delete animations
	for(int i=0; i<m_animations.size(); i++) delete m_animations[i];
	m_animations.clear();
	changedAnimation(0);

	//delete parts
	for(QMap<int,Part*>::iterator i=m_parts.begin(); i!=m_parts.end(); i++) {
		m_scene.removeItem( *i );
		delete *i;
	}
	m_parts.clear();
	changedPart(0);

	//delete controllers
	foreach(IKController* c, m_controllers) delete c;
	m_controllers.clear();
	changedController(-1);


	//Reset other values
	m_partValue = 1;
	m_file = QString::null;
	m_scene.setSceneRect( QRectF() );
}

