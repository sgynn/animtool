#ifndef _IK_
#define _IK_

#include "part.h"
class View;

class IKController {
	public:
	IKController(int, Part*, Part*, Part*, Part*);

	/** Apply rotations from IK data */
	void apply(View* v) const;

	int   getID() const { return m_id; }
	Part* getPartA() const { return m_partA; }
	Part* getPartB() const { return m_partB; }
	Part* getHead() const { return m_head; }
	Part* getGoal() const { return m_goal; }

	/** Generate name from parts */
	QString getName() const;

	protected:
	int   m_id;
	Part* m_partA;
	Part* m_partB;
	Part* m_head;
	Part* m_goal;
};


#endif

