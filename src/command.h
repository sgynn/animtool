#ifndef _COMMAND_
#define _COMMAND_

#include <QObject>
#include <QList>

class Part;
class Frame;
class Project;
class Command;
class CommandGroup;
class QAction;

/** Alternative method. All editing occurs via commands */
class CommandStack : public QObject {
	friend class Command;
	Q_OBJECT;
	public:
	CommandStack(QObject* parent);	// Constructor
	~CommandStack();		// Destructor
	void setProject(Project* p) { m_project=p; }
	void setActions(QAction* undo=0, QAction* redo=0);

	void push(Command* cmd, bool execute=true);	// Push and execute a new command to the list
	void begin(const QString& text=QString::null);	// Begin command group (undoes as one)
	void end(bool execute=true);			// End command group

	bool canUndo() const { return m_stack.size(); }
	bool canRedo() const { return m_redo.size(); }
	bool isClean() const { return m_stack.size()==m_clean; }

	public slots:
	void undo();					// Undo last command
	void redo();					// Redo next command
	void setClean();				// Set the clean state to this
	void clear();					// Clear command stacks
	void breakChain();				// last comand will not combine

	//Debug info
	void dumpStack();

	signals:
	void updateTable();				// Signal table data has changed
	void updateFrame(int);				// Signal frame has changed in table
	void updateView();				// Signal view to redraw
	void updatePart(Part* part, const Frame& data);	// Signal that part data has been modified
	void skipEvents(bool);				// Skip element changed events

	protected:
	Project* m_project;		// Current project
	QList<Command*> m_stack;	// Undo stack
	QList<Command*> m_redo;		// Redo stack
	int m_clean;			// Index of clean state
	CommandGroup* m_group;		// Current command group ( uses begin() and end() )

	QAction* m_undoAction;		// Undo menu action
	QAction* m_redoAction;		// Redo menu action
	void updateActions();
};

/** Command base class */
class Command {
	friend class CommandStack;
	friend class CommandGroup;
	public:
	Command() : m_stack(0), m_project(0), m_chain(true) {}
	virtual ~Command() {}

	virtual QString text() const { return ""; }		// Command text

	virtual void execute() = 0;				// Execute command
	virtual void undo() = 0;				// Execute reverse command
	virtual bool combine(Command* next) { return false; }	// Combine a subsequent command with this.

	protected:
	Project* project() const { return m_project; }		// Get current project
	virtual int typeID() const { return -1; }		// Type id for combining commands

	// Signals processed from parent stack - need to wrap them as signals are protected
	void updateTable()				{ m_stack->updateTable(); }
	void updateFrame(int f)				{ m_stack->updateFrame(f); }
	void updateView()				{ m_stack->updateView(); }
	void updatePart(Part* part, const Frame& data)	{ m_stack->updatePart(part,data); }
	void skipEvents(bool skip)			{ m_stack->skipEvents(skip); }

	private:
	CommandStack* m_stack;					// Parent stack
	Project* m_project;					// Current project
	bool m_chain;						// Can this command be appended to
	virtual void setProject(Project* p) { m_project=p; }	// Set current project
};

/** Group of commands. Use CommandStack::begin() and end() to use */
class CommandGroup : public Command {
	friend class CommandStack;
	public:
	CommandGroup(const QString& text=QString::null, bool fwd=false);
	~CommandGroup();					// Delete commands
	virtual QString text() const { return m_text; }		// Command text
	virtual void execute();					// Execute all commands in order
	virtual void undo();					// Execute command undo() functions in reverse order
	void push(Command* command);				// Push a command to the list
	protected:
	QList<Command*> m_list;					// Commands
	QString m_text;
	bool m_fwd;						// Undo does not execute in reverse order
	virtual void setProject(Project* p);			// Set current project
};

#endif

