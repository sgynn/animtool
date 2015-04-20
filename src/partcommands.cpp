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
	Part* part = project()->createPart(m_name, m_part);
	part->setImage( project()->loadGraphic( m_source ) );
	if(m_source!=QString::null) part->setSource(m_source);
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
	//Create part
	Part* part = project()->createPart(old->getName()+"_copy");
	part->setImage( old->pixmap() );
	part->setSource( old->getSource() );
	part->setOffset( old->offset() );
	part->setRest( old->rest() );
	part->setHidden( old->hidden() );
	project()->addPart( part, old->getParent() );
	//TODO Clone all keyframes
}
void ClonePart::undo() {
	project()->removePart( getPart() );
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







