#ifndef _PART_COMMANDS_
#define _PART_COMMANDS_

#include "command.h"

class QTreeView;

class PartCommand : public Command {
	public:
	PartCommand(int partID=0) : m_part(partID) {}
	PartCommand(Part* part);
	protected:
	Part* getPart() const;
	int m_part;
};

class CreatePart : public PartCommand {
	public:
	CreatePart(const QString& name, const QString& source, int parent=0, int id=0);
	CreatePart(const QString& name, const QString& source, Part* parent, int id=0);
	QString text() const { return "create part"; }
	void execute();
	void undo();
	protected:
	QString m_name;		// Part name
	QString m_source;	// Part image file name
	int m_parent;		// Parent id
};

class ClonePart : public PartCommand {
	public:
	ClonePart(Part* part);
	QString text() const { return "clone part"; }
	void execute();
	void undo();
	protected:
	int m_newPart;
};

class CloneHeirachy : public PartCommand {
	public:
	CloneHeirachy(Part* part);
	QString text() const { return "clone heirachy"; }
	void execute();
	void undo();
	protected:
	void cloneChildren(Part* src, Part* dest);
	QList<int> m_parts;
};

class DeletePart : public CreatePart {
	public:
	DeletePart(Part* part);
	QString text() const { return "delete part"; }
	void execute() { CreatePart::undo(); }
	void undo() { CreatePart::execute(); }
};

class RenamePart : public PartCommand {
	public:
	RenamePart(Part* part, const QString& name, QTreeView* list);
	QString text() const { return "rename part"; }
	void execute()	{ rename(m_newName); }
	void undo()	{ rename(m_oldName); }
	protected:
	void rename(const QString& name);
	QString m_oldName;
	QString m_newName;
	QTreeView* m_list;
};

class MovePart : public PartCommand {
	public:
	MovePart(Part* part, Part* newParent, QTreeView* tree);
	QString text() const { return "move part"; }
	void execute()	{ move(m_newParent); }
	void undo()	{ move(m_oldParent); }
	protected:
	void move(int parent);
	int m_oldParent;
	int m_newParent;
	QTreeView* m_list;
};

class ChangeZOrder : public PartCommand {
	public:
	ChangeZOrder(Part* part, int z);
	QString text() const { return "change Z order"; }
	void execute();
	void undo();
	protected:
	void setZ(int z, bool behind);
	int m_oldZ, m_newZ;
};

#endif

