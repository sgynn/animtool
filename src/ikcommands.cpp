#include "ikcommands.h"
#include "project.h"
#include "animation.h"
#include "ik.h"

//// //// //// //// //// //// //// //// Create / Set //// //// //// //// //// //// //// ////

SetController::SetController(int id, int a, int b, int h, int g) 
	: m_id(id), m_a(a), m_b(b), m_head(h), m_goal(g) {
}

void SetController::execute() {
	Part* a = project()->getPart(m_a);
	Part* b = project()->getPart(m_b);
	Part* h = project()->getPart(m_head);
	Part* g = project()->getPart(m_goal);
	m_id = project()->setController(m_id, a,b,h,g);
}
void SetController::undo() {
	project()->removeController(m_id);
}

//// //// //// //// //// //// //// //// Destroy //// //// //// //// //// //// //// ////

DeleteController::DeleteController(IKController* c) : SetController(c->getID(), 0,0,0,0) { 
	m_a = c->getPartA()->getID();
	m_b = c->getPartB()? c->getPartB()->getID(): 0;
	m_head = c->getHead()->getID();
	m_goal = c->getGoal()->getID();
}
void DeleteController::execute() {
	SetController::undo();
}
void DeleteController::undo() {
	SetController::execute();
}

//// //// //// //// //// //// //// //// Change Order //// //// //// //// //// //// //// ////

ChangeControllerOrder::ChangeControllerOrder(int a, int b) : m_a(a), m_b(b) {
}
void ChangeControllerOrder::execute() {
	project()->swapControllers(m_a, m_b);
}
void ChangeControllerOrder::undo() {
	execute();
}


//// //// //// //// //// //// //// //// Change State //// //// //// //// //// //// //// ////

ChangeControllerState::ChangeControllerState(Animation* a, int id, bool state) 
	: m_animation(a->getID()), m_controller(id), m_state(state) {
}

void ChangeControllerState::execute() {
	Animation* a = project()->getAnimation(m_animation);
	a->setControllerState(m_controller, m_state);
}

void ChangeControllerState::undo() {
	Animation* a = project()->getAnimation(m_animation);
	a->setControllerState(m_controller, !m_state);
}





