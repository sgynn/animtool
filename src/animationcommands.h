#ifndef _ANIMATION_COMMANDS_
#define _ANIMATION_COMMANDS_

#include "command.h"
#include "animation.h"

class QListView;

class AnimationCommand : public Command {
	public:
	AnimationCommand() : m_animation(0) {}
	AnimationCommand(Animation* anim) : m_animation( anim->getID() ) {}
	protected:
	Animation* getAnimation();
	int m_animation;
};

class AddAnimation : public AnimationCommand {
	public:
	AddAnimation(const QString& name);
	QString text() const { return "add animation"; }
	void execute();
	void undo();
	protected:
	QString m_name;
};

class CloneAnimation : public AnimationCommand {
	public:
	CloneAnimation(Animation* anim);
	QString text() const { return "clone animation"; }
	void execute();
	void undo();
	private:
	int m_template;
};

class DeleteAnimation : public AnimationCommand {
	public:
	DeleteAnimation(Animation* anim);
	QString text() const { return "delete animation"; }
	void execute();
	void undo();
	private:
	QString m_name;
	//Need to save animation data
	struct Keyframe { int part; Frame data; };
	QList<Keyframe> m_keys;
};

class RenameAnimation : public AnimationCommand {
	public:
	RenameAnimation(Animation* anim, const QString& name, QListView* list);
	QString text() const { return "rename animation"; }
	void execute()	{ rename(m_newName); }
	void undo()	{ rename(m_oldName); }
	private:
	void rename(const QString& name);
	QString m_oldName, m_newName;
	QListView* m_list;
};

class InsertFrame : public AnimationCommand {
	public:
	InsertFrame(Animation* anim, int frame);
	QString text() const { return "insert frame"; }
	void execute();
	void undo();
	private:
	int m_frame;
};

class DeleteFrame : public AnimationCommand {
	public:
	DeleteFrame(Animation* anim, int frame);
	QString text() const { return "delete frame"; }
	void execute();
	void undo();
	private:
	int m_frame;
	QList<Frame> m_keys;	// Need to store deleted keyframes
};

class SetFrames : public AnimationCommand {
	public:
	SetFrames(Animation* anim, int number);
	QString text() const { return "change frame count"; }
	void execute();
	void undo();
	int typeID() const { return 0x30; }
	bool combine(Command* next);
	private:
	int m_oldCount, m_newCount;
};


#endif

