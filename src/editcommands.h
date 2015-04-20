#ifndef _EDIT_COMMANDS_
#define _EDIT_COMMANDS_

#include "partcommands.h"
#include "animation.h"


class ChangeRest : public PartCommand {
	public:
	ChangeRest(Part* part, const QPointF& pivot, const QPointF& rest, bool hidden);
	ChangeRest(Part* part, const QPointF& data, int index);
	ChangeRest(Part* part, bool hidden);
	QString text() const { return "rest data"; }
	void execute()	{ setRest(m_newPivot, m_newRest, m_changeFlags); }
	void undo()	{ setRest(m_oldPivot, m_oldRest, m_changeFlags); }
	int typeID() const { return 0x10; }
	bool combine(Command* next);
	private:
	void setRest(const QPointF& pivot, const QPointF& rest, int flags);
	QPointF m_oldRest, m_newRest;
	QPointF m_oldPivot, m_newPivot;
	int m_changeFlags;
};

class ChangeFrameData : public PartCommand {
	public:
	ChangeFrameData(Animation* anim, Part* part, const Frame& oldData, const Frame& newData);
	ChangeFrameData(Animation* anim, Part* part, int frame, int oldKey, int newKey);
	ChangeFrameData(Animation* anim, Part* part, int frame, float oldAngle, float newAngle);
	ChangeFrameData(Animation* anim, Part* part, int frame, const QPointF& oldPos, const QPointF& newPos);
	ChangeFrameData(Animation* anim, Part* part, int frame, bool oldVis, bool newVis);
	QString text() const { return "change"; }
	void execute()	{ setData(m_new, m_mask); }
	void undo()	{ setData(m_old, m_mask); }
	int typeID() const { return 0x11; }
	bool combine(Command* next);
	private:
	int m_animation;
	void setData(Frame& data, int mask);
	int m_mask;
	Frame m_old, m_new;
};

class ChangePartOrder : public PartCommand {
	public:
	ChangePartOrder(Part* part, Part* before);
	QString text() const { return "change order"; }
	void execute();
	void undo();
	int typeID() const { return 0x14; }
	bool combine(Command* next);
	private:
};


#endif

