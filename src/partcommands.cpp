#include "partcommands.h"
#include "project.h"
#include "part.h"
#include "animtool.h"

#include <assert.h>
#include <QStandardItem>

PartCommand::PartCommand(Part* part) : m_part(part->getID()) {
}
Part* PartCommand::getPart() const {
	Part* part = project()->getPart(m_part);
	assert( part && " Failed to locate part " );
	return part;
}

//// //// //// //// //// //// //// //// Create Part //// //// //// //// //// //// //// ////

CreatePart::CreatePart(const QString& name, const QString& source, int parent, int id) {
	m_name = name;
	m_source = source;
	m_parent = parent;
	m_part = id;
}
CreatePart::CreatePart(const QString& name, const QString& source, Part* parent, int id) {
	m_name = name;
	m_source = source;
	m_parent = parent? parent->getID(): 0;
	m_part = id;
}
void CreatePart::execute() {
	bool nullPart = m_source == QString::null;
	Part* part = project()->createPart(m_name, m_part, nullPart);
	if(nullPart) {
		part->setImage( QPixmap(":/icon/res/null.png") );
		part->setOffset( QPointF(-12,-12) );
	} else {
		part->setImage( project()->loadGraphic( m_source ) );
		part->setSource(m_source);
	}
	m_part = part->getID(); // Save ID in case it was not set
	//Add to project
	Part* parent = project()->getPart( m_parent );
	project()->addPart( part, parent );
}
void CreatePart::undo() {
	
	project()->removePart( getPart() );
}

//// //// //// //// //// //// //// //// Clone Part //// //// //// //// //// //// //// ////

ClonePart::ClonePart(Part* part) : PartCommand(part) { }
void ClonePart::execute() {
	Part* old = getPart();
	Part* p = project()->clonePart(old, old->getParent());
	m_newPart = p->getID();
}
void ClonePart::undo() {
	Part* part = project()->getPart(m_newPart);
	project()->removePart( part );
}

//// //// //// //// //// //// //// //// Clone Heirachy //// //// //// //// //// //// //// ////

CloneHeirachy::CloneHeirachy(Part* part) : PartCommand(part) {}
void CloneHeirachy::execute() {
	Part* first = getPart();
	Part* p = project()->clonePart(first, first->getParent());
	m_parts.push_back(p->getID());
	cloneChildren(first, p);
}
void CloneHeirachy::cloneChildren(Part* src, Part* dst) {
	foreach(Part* c, src->children()) {
		Part* p = project()->clonePart(c, dst);
		m_parts.push_back(p->getID());
		cloneChildren(c, p);
	}
}
void CloneHeirachy::undo() {
	foreach(int i, m_parts) {
		Part* part = project()->getPart(i);
		project()->removePart(part);
	}
}

//// //// //// //// //// //// //// //// Delete Part //// //// //// //// //// //// //// ////

DeletePart::DeletePart(Part* part) : CreatePart(part->getName(), part->getSource(), part->getParent()? part->getParent()->getID():0, part->getID()) { }

//// //// //// //// //// //// //// //// Rename Part //// //// //// //// //// //// //// ////

RenamePart::RenamePart(Part* part, const QString& name, QTreeView* list) : PartCommand(part), m_list(list) {
	m_oldName = part->getName();
	m_newName = name;
}
void RenamePart::rename(const QString& name) {
	printf("Rename part %d to %s\n", m_part, name.toAscii().data());
	Part* part = getPart();
	//Rename part in list widget
	QStandardItem* item = AnimTool::findItem(m_list->model(), part->getName(), part->getID());
	if(item && item->text()!=name) {
		skipEvents(true);
		item->setText( name );
		skipEvents(false);
	}
	//rename part
	part->setName( name );
}

//// //// //// //// //// //// //// //// Move Part //// //// //// //// //// //// //// ////

MovePart::MovePart(Part* part, Part* newParent, QTreeView* list) : PartCommand(part), m_list(list) {
	m_oldParent = part->getParent()? part->getParent()->getID(): 0;
	m_newParent = newParent? newParent->getID(): 0;
}
void MovePart::move(int parentID) {
	Part* part = getPart();
	Part* parent = project()->getPart( parentID );
	part->setParent( parent );

	//Move in view
	QStandardItem* root = static_cast<QStandardItemModel*>( m_list->model() )->invisibleRootItem();
	QStandardItem* item = AnimTool::findItem(m_list->model(), part->getName(), part->getID());
	assert(item && " Invalid partList item " );
	QStandardItem* parentItem = item->parent();
	if(parentItem==0 && parentID==0) return; // Nothing to do
	if(parentItem && parentItem->data().toInt()==parentID) return; //Nothing to do
	if(!parentItem) parentItem = root;
	skipEvents(true);
	//Remove item
	QList<QStandardItem*> items = parentItem->takeRow( item->row() );
	//Add Item
	if(parentID) parentItem = AnimTool::findItem(m_list->model(), parentID);
	else parentItem = root;
	parentItem->appendRow( items );
	skipEvents(false);
}



//// //// //// //// //// //// //// //// Z Order //// //// //// //// //// //// //// ////

ChangeZOrder::ChangeZOrder(Part* part, int z) : PartCommand(part), m_oldZ(-1), m_newZ(z) {
}
void ChangeZOrder::execute() {
	m_oldZ = project()->scene()->items().indexOf( getPart() );
	setZ( m_newZ, m_newZ > m_oldZ );
}
void ChangeZOrder::undo() {
	setZ( m_oldZ, m_oldZ > m_newZ );
}
void ChangeZOrder::setZ(int z, bool behind) {
	QGraphicsItem* itm = project()->scene()->items()[z];
	if(behind) getPart()->stackBefore( itm );
	else itm->stackBefore( getPart() );
	project()->scene()->invalidate( project()->scene()->sceneRect() );
}


