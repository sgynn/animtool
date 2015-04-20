#include "animationcommands.h"
#include "animation.h"
#include "project.h"

#include <assert.h>

#include <QStandardItem>
#include "animtool.h"


Animation* AnimationCommand::getAnimation() {
	Animation* anim = project()->getAnimation( m_animation );
	assert(anim && " Failed to find animation " );
	return anim;
}

//// //// //// //// //// //// //// //// Add Animation //// //// //// //// //// //// //// ////

AddAnimation::AddAnimation(const QString& name) : m_name(name) { }
void AddAnimation::execute() {
	Animation* anim = new Animation();
	anim->setName( m_name );
	project()->addAnimation( anim, m_animation );
	m_animation = anim->getID();
}
void AddAnimation::undo() {
	project()->removeAnimation( getAnimation() );
}

///// //// //// //// //// //// //// //// Add Animation //// //// //// //// //// //// //// ////

CloneAnimation::CloneAnimation(Animation* anim) : m_template(anim->getID()) { }
void CloneAnimation::execute() {
	Animation* other = project()->getAnimation( m_template );
	Animation* anim = new Animation( *other );
	anim->setName( other->name() + "_copy" );
	project()->addAnimation( anim, m_animation );
	m_animation = anim->getID();
}
void CloneAnimation::undo() {
	project()->removeAnimation( getAnimation() );
}

/// //// //// //// //// //// //// //// Delete Animation //// //// //// //// //// //// //// ////

DeleteAnimation::DeleteAnimation(Animation* anim) : AnimationCommand(anim) { }
void DeleteAnimation::execute() {
	//Save keyframes
	Animation* anim = getAnimation();
	m_name = anim->name();
	QList<int> parts = anim->parts();
	for(int i=0; i<parts.size(); i++) {
		Part* part = project()->getPart( parts[i] );
		for(int f=0; f<anim->frameCount(); f++) {
			if(anim->isKeyframe(f, part)) {
				Keyframe key;
				key.part = parts[i];
				key.data = anim->frameData(f, part);
				m_keys.push_back(key);
			}
		}
	}
	//Delete animation
	project()->removeAnimation( anim );
}
void DeleteAnimation::undo() {
	//Create the animation
	Animation* anim = new Animation();
	anim->setName( m_name );
	//Set keyframes
	for(int i=0; i<m_keys.size(); i++) {
		Part* part = project()->getPart( m_keys[i].part );
		anim->setKeyframe(m_keys[i].data.frame, part, m_keys[i].data);
	}
	//Add to list
	project()->addAnimation( anim, m_animation );
}


//// //// //// //// //// //// //// //// Rename Animation //// //// //// //// //// //// //// ////

RenameAnimation::RenameAnimation(Animation* anim, const QString& name, QListView* list) : AnimationCommand(anim), m_list(list) {
	m_oldName = anim->name();
	m_newName = name;
}
void RenameAnimation::rename(const QString& name) {
	Animation* anim = getAnimation();
	//Change value in treeview
	skipEvents(true);
	QStandardItem* item = AnimTool::findItem(m_list->model(), anim->getID());
	item->setText( name );
	m_list->model()->sort(0); //Sort list?
	skipEvents(false);
	// Set name
	anim->setName( name );
}

//// //// //// //// //// //// //// //// Insert Frame //// //// //// //// //// //// //// ////

InsertFrame::InsertFrame(Animation* anim, int frame) : AnimationCommand(anim), m_frame(frame) {}
void InsertFrame::execute() {
	getAnimation()->insertFrame(m_frame);
	project()->setCurrent( getAnimation() );
	updateTable();
}
void InsertFrame::undo() {
	getAnimation()->deleteFrame(m_frame);
	project()->setCurrent( getAnimation() );
	updateTable();
}


//// //// //// //// //// //// //// //// Delete Frame //// //// //// //// //// //// //// ////

DeleteFrame::DeleteFrame(Animation* anim, int frame) : AnimationCommand(anim), m_frame(frame) {}
void DeleteFrame::execute() {
	//Save keys
	Animation* anim = getAnimation();
	QList<int> parts = anim->parts();
	for(int i=0; i<parts.size(); i++) {
		Part* part = project()->getPart( parts[i] );
		if(anim->isKeyframe(m_frame, part)) {
			Frame data = anim->frameData(m_frame, part);
			data.frame = parts[i];
			m_keys.push_back(data);
		}
	}
	//delete frame
	anim->deleteFrame(m_frame);
	project()->setCurrent( anim );
	updateTable();
	
}
void DeleteFrame::undo() {
	Animation* anim = getAnimation();
	//insert frame
	anim->insertFrame(m_frame);
	//Add keys
	for(int i=0; i<m_keys.size(); i++) {
		Part* part = project()->getPart( m_keys[i].frame );
		anim->setKeyframe(m_frame, part, m_keys[i]);
	}
	project()->setCurrent( anim );
	updateTable();
}

//// //// //// //// //// //// //// //// Delete Frame //// //// //// //// //// //// //// ////

SetFrames::SetFrames(Animation* anim, int number) : AnimationCommand(anim) {
	m_oldCount = anim->frameCount();
	m_newCount = number;
}
void SetFrames::execute() {
	getAnimation()->setFrameCount(m_newCount);
	project()->setCurrent( getAnimation() );
	updateTable();

}
void SetFrames::undo() {
	getAnimation()->setFrameCount(m_oldCount);
	project()->setCurrent( getAnimation() );
	updateTable();
}

bool SetFrames::combine(Command* next) {
	SetFrames* n = static_cast<SetFrames*>(next);
	if(m_animation != n->m_animation) return false;
	m_newCount = n->m_newCount;
	return true;
}

