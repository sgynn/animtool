#include "ik.h"
#include "view.h"
#include <cmath>

static const QPointF zero(0,0);

IKController::IKController(int id, Part* a, Part* b, Part* h, Part* g)
	: m_id(id), m_partA(a), m_partB(b), m_head(h), m_goal(g) {
}

QString IKController::getName() const {
	QString s = m_partA->getName();
	if(m_partB) s += " - " + m_partB->getName();
	s += " -> " + m_goal->getName();
	return s;
}

inline float distance(const QPointF& a, const QPointF& b) {
	return sqrt( (a.x()-b.x()) * (a.x()-b.x()) + (a.y()-b.y())*(a.y()-b.y()) );
}

float calculateAngle(const QPointF& dir, const QPointF& goal) {
	float rt = atan2(dir.x(), dir.y());
	float rg = atan2(goal.x(), goal.y());
	return (rt - rg) * 180 / 3.141592654;
}

void lookat(View* v, Part* pa, Part* pb, const QPointF& goal) {
	QPointF d = pa->mapFromScene(pb->pos());
	float ang = calculateAngle(d, goal - pa->pos());
	v->setAbsoluteRotation( pa, ang );
}

void IKController::apply(View* v) const {
	Part* parent = m_partA->getParent();
	// Simple lookat mode
	if(m_partB == 0) {
		lookat(v, m_partA, m_head, m_goal->pos());
	}
	// Two bone ik
	else {

		// Calculate lengths
		float la = distance(m_partA->pos(), m_partB->pos());
		float lb = distance(m_partB->pos(), m_head->pos());
		float lg = distance(m_partA->pos(), m_goal->pos());

		// Target out of range - lookat
		if(lg >= la + lb) {
			lookat(v, m_partA, m_partB, m_goal->pos());
			lookat(v, m_partB, m_head, m_goal->pos());
		}
		// Target too close
		else if(lg <= fabs(la-lb)) {
			QPointF otherWay = m_partA->pos() + (m_partA->pos() - m_goal->pos());
			lookat(v, m_partA, m_partB, otherWay);
			lookat(v, m_partB, m_head, m_goal->pos());
		}
		// In range - ik
		else if(lg > 0) {
			QPointF a = m_partA->pos();
			QPointF b = m_partB->pos() - a;
			QPointF c = m_head->pos() - b;
			QPointF g = m_goal->pos() - a;

			float d = (lg*lg + la*la - lb*lb) / (2*lg);
			float r = sqrt(la*la - d*d);
			
			QPointF n(g.y()/lg, -g.x()/lg);
			QPointF r0 = a + g * (d/lg) + n*r;
			QPointF r1 = a + g * (d/lg) - n*r;

			// Use other goal?
			if(b.x()*n.x() + b.y()*n.y() < 0) r0 = r1;

			lookat(v, m_partA, m_partB, r0);
			lookat(v, m_partB, m_head, m_goal->pos());

			// ToDo: Limits
		}
	}
}

