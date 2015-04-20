#include "command.h"
#include <cstdio>

#include <QAction>


CommandStack::CommandStack(QObject* parent): QObject(parent), m_clean(0), m_group(0), m_undoAction(0), m_redoAction(0) {
}
CommandStack::~CommandStack() {
	clear();
}

void CommandStack::setActions(QAction* undo, QAction* redo) {
	m_undoAction = undo;
	m_redoAction = redo;
}


void CommandStack::push(Command* cmd, bool execute) {
	//Set command data
	cmd->m_stack = this;
	cmd->setProject( m_project );

	//If a command group is active, use that instead
	if(m_group) { m_group->push(cmd); return; }

	//Invalidate clean state if unreachable, and clear redo stack
	if(m_clean > m_stack.size()) m_clean = -1;
	for(int i=0; i<m_redo.size(); i++) delete m_redo[i];
	m_redo.clear();

	//Execute action
	if(execute) cmd->execute();

	//Combine commands or Add to stack
	Command* last = m_stack.size()? m_stack.back(): 0;
	if(last && last->typeID()==cmd->typeID() && last->m_chain && last->combine(cmd)) {
		delete cmd;
	} else {
		if(last) last->m_chain = false; //break chain
		m_stack.push_back( cmd );
	}

	//Update action state and text
	updateActions();
}

void CommandStack::begin(const QString& name) {
	if(m_group) printf("Warning - Command group already active\n");
	else m_group = new CommandGroup( name );
}

void CommandStack::end(bool execute) {
	if(!m_group) printf("Error - No command group active\n");
	else {
		CommandGroup* group = m_group;
		m_group = 0;
		push(group, execute);
	}
}



void CommandStack::undo() {
	if(m_stack.size()) {
		Command* cmd = m_stack.back();
		printf("Undo: %s\n", cmd->text().toAscii().data());
		cmd->undo();
		//Move to redo stack
		m_stack.pop_back();
		m_redo.push_back( cmd );
		updateActions();
	}
}
void CommandStack::redo() {
	if(m_redo.size()) {
		Command* cmd = m_redo.back();
		printf("Redo: %s\n", cmd->text().toAscii().data());
		cmd->execute();
		//Move to undo stack
		m_redo.pop_back();
		m_stack.push_back( cmd );
		updateActions();
	}

}
void CommandStack::setClean() {
	m_clean = m_stack.size();
}
void CommandStack::clear() {
	for(int i=0; i<m_stack.size(); i++) delete m_stack[i];
	for(int i=0; i<m_redo.size(); i++) delete m_redo[i];
	m_stack.clear();
	m_redo.clear();
	m_clean = -1;
}
void CommandStack::breakChain() {
	if(m_stack.size()) m_stack.back()->m_chain = false;
}

void CommandStack::updateActions() {
	if(m_undoAction) {
		m_undoAction->setEnabled( canUndo() );
		m_undoAction->setText( canUndo()? "&Undo " + m_stack.back()->text(): "&Undo");
	}
	if(m_redoAction) {
		m_redoAction->setEnabled( canRedo() );
		m_redoAction->setText( canRedo()? "&Redo " + m_redo.back()->text(): "&Redo");
	}
}

void CommandStack::dumpStack() {
	printf("\nUndo Stack: %d+%d commands:\n", m_stack.size(), m_redo.size());
	int k = m_stack.size() + m_redo.size()-1;
	for(int i=0; i<m_stack.size(); i++)   printf("%c %s\n", m_clean==i?'*':' ', m_stack[i]->text().toAscii().data());
	for(int i=m_redo.size()-1; i>=0; i--) printf("%c%c%s\n",m_clean==k-i?'*':' ', i==m_redo.size()-1?'>':' ', m_redo[i]->text().toAscii().data());
}

//// //// //// //// //// //// //// //// Command Group //// //// //// //// //// //// //// //// 

CommandGroup::CommandGroup(const QString& text, bool fwd) : m_text(text), m_fwd(fwd) {}
CommandGroup::~CommandGroup() {
	for(int i=0; i<m_list.size(); i++) delete m_list[i];
}
void CommandGroup::execute() {
	for(int i=0; i<m_list.size(); i++) m_list[i]->execute();
}
void CommandGroup::undo() {
	if(m_fwd) for(int i=0; i<m_list.size(); i++) m_list[i]->undo();
	else for(int i=m_list.size()-1; i>=0; i--) m_list[i]->undo();
}
void CommandGroup::push(Command* cmd) {
	//Combine commands or Add to list
	Command* last = m_list.size()? m_list.back(): 0;
	if(last && last->typeID()==cmd->typeID() && last->m_chain && last->combine(cmd)) {
		delete cmd;
	} else {
		if(last) last->m_chain = false; //break chain
		m_list.push_back( cmd );
	}
}
void CommandGroup::setProject(Project* p) {
	m_project = p;
	for(int i=0; i<m_list.size(); i++) {
		m_list[i]->setProject(p);
		m_list[i]->m_stack = m_stack;
	}
}



