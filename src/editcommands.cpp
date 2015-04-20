#include "editcommands.h"
#include "animation.h"
#include "part.h"
#include "project.h"

ChangeRest::ChangeRest(Part* part, const QPointF& pivot, const QPointF& rest, bool hidden) : PartCommand(part) {
	m_oldPivot = part->offset();
	m_oldRest  = part->rest();
	m_newPivot = pivot;
	m_newRest = rest;
	m_changeFlags  = m_oldPivot != m_newPivot? 1: 0;
	m_changeFlags |= m_oldRest  != m_newRest?  2: 0;
	m_changeFlags |= part->hidden() != hidden? 4: 0;
}
ChangeRest::ChangeRest(Part* part, const QPointF& data, int index) : PartCommand(part), m_changeFlags(0) {
	switch(index) {
	case 1: 
		m_oldPivot = part->offset();
		m_newPivot = data;
		m_changeFlags = 1;
		break;
	case 2:
		m_oldRest = part->rest();
		m_newRest = data;
		m_changeFlags = 2;
		break;
	}
}
ChangeRest::ChangeRest(Part* part, bool hidden) : PartCommand(part) {
	m_changeFlags = part->hidden() != hidden? 4: 0;
}
bool ChangeRest::combine(Command* next) {
	ChangeRest* cmd = static_cast<ChangeRest*>(next);
	if(m_part != cmd->m_part) return false; //Not the same part
	if(cmd->m_changeFlags&1) m_newPivot = cmd->m_newPivot;
	if(cmd->m_changeFlags&2) m_newRest  = cmd->m_newRest;
	m_changeFlags |= cmd->m_changeFlags&3;
	//Hidden
	m_changeFlags ^= cmd->m_changeFlags&4;
	return true;
}

void ChangeRest::setRest(const QPointF& pivot, const QPointF& rest, int flags) {
	Part* part = getPart();
	// Compile frameData to set
	Frame data;
	data.frame = -1; // Flag that this is temporary
	data.angle = part->localAngle();
	data.offset = part->localOffset();
	data.visible = part->isVisible();
	if(!project()->currentAnimation()) data.offset = QPointF(); //TEST
	// Set rest data
	if(flags&1) part->setOffset( pivot );
	if(flags&2) part->setRest( rest );
	if(flags&4) part->setHidden( !part->hidden() );
	// If hidden has changed, calculate visibile
	if(flags&4) data.visible = !part->hidden(); //FIXME should calculate visibility
	// Fire update events
	updatePart(part, data);
}

//// //// //// //// //// //// //// //// Frame Data //// //// //// //// //// //// //// ////

ChangeFrameData::ChangeFrameData(Animation* anim, Part* part, const Frame& oldData, const Frame& newData) : PartCommand(part) {
	m_animation = anim? anim->getID(): 0;
	m_old = oldData;
	m_new = newData;
	m_mask = 0xf;
}
ChangeFrameData::ChangeFrameData(Animation* anim, Part* part, int frame, int oldKey, int newKey) : PartCommand(part) {
	m_animation = anim? anim->getID(): 0;
	m_old.frame = m_new.frame = frame;
	m_old.mode = oldKey;
	m_new.mode = newKey;
	m_mask = 1;
}
ChangeFrameData::ChangeFrameData(Animation* anim, Part* part, int frame, float oldAngle, float newAngle) : PartCommand(part) {
	m_animation = anim? anim->getID(): 0;
	m_old.frame = m_new.frame = frame;
	m_old.angle = oldAngle;
	m_new.angle = newAngle;
	m_mask = 2;
}
ChangeFrameData::ChangeFrameData(Animation* anim, Part* part, int frame, const QPointF& oldPos, const QPointF& newPos) : PartCommand(part) {
	m_animation = anim? anim->getID(): 0;
	m_old.frame = m_new.frame = frame;
	m_old.offset = oldPos;
	m_new.offset = newPos;
	m_mask = 4;
}
ChangeFrameData::ChangeFrameData(Animation* anim, Part* part, int frame, bool oldVis, bool newVis) : PartCommand(part) {
	m_animation = anim? anim->getID(): 0;
	m_old.frame = m_new.frame = frame;
	m_old.visible = oldVis;
	m_new.visible = newVis;
	m_mask = 8;
}
bool ChangeFrameData::combine(Command* next) {
	ChangeFrameData* cmd = static_cast<ChangeFrameData*>(next);
	if(m_part!=cmd->m_part || m_animation!=cmd->m_animation || m_new.frame != cmd->m_new.frame) return false;
	if(cmd->m_mask & 1) m_new.mode  = cmd->m_new.mode;
	if(cmd->m_mask & 2) m_new.angle = cmd->m_new.angle;
	if(cmd->m_mask & 4) m_new.offset = cmd->m_new.offset;
	if(cmd->m_mask & 8) m_new.visible = cmd->m_new.visible;
	m_mask |= cmd->m_mask;
	return true;
}

void ChangeFrameData::setData(Frame& data, int mode) {
	// This function does _everything_
	Animation* anim = project()->getAnimation(m_animation);
	Part* part = getPart();
	int frame = m_old.frame;
	int key = anim? anim->isKeyframe(frame, part): 0;

	//Need to fill in the rest of data
	if(~mode&1) data.mode = key;
	if(~mode&2) data.angle = part->localAngle();
	if(~mode&4) data.offset = part->localOffset();
	if(~mode&8) data.visible = part->isVisible();

	//Set animation key frame
	if((mode&1) || key) anim->setKeyframe(frame, part, data);
	if(mode&1) updateFrame(frame);
	
	//Update part
	updatePart(part, data);
}





