#ifndef _IK_COMMANDS_
#define _IK_COMMANDS_

#include "command.h"

class IKController;
class Animation;

class SetController : public Command {
	public:
	SetController(int id, int a, int b, int h, int g);
	QString text() const { return "add controller"; }
	void execute();
	void undo();
	protected:
	int m_id;
	int m_a, m_b;
	int m_head, m_goal;
};

class DeleteController : public SetController {
	public:
	DeleteController(IKController*);
	QString text() const { return "remove controller"; }
	void execute();
	void undo();
};

class ChangeControllerOrder : public Command {
	public:
	ChangeControllerOrder(int indexA, int indexB);
	QString text() const { return "move controller"; }
	void execute();
	void undo();
	protected:
	int m_a, m_b;
};

class ChangeControllerState : public Command {
	public:
	ChangeControllerState(Animation*, int, bool);
	QString text() const { return "controller state"; }
	void execute();
	void undo();
	protected:
	int m_animation;
	int m_controller;
	bool m_state;
};



#endif

