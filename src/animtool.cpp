#include <QtGui> //all qt stuff
#include "animtool.h"
#include "part.h"
#include "animation.h"
#include "export.h"

#include <cstdio>
#include <assert.h>

#include "partcommands.h"
#include "animationcommands.h"
#include "editcommands.h"

AnimTool::AnimTool(QWidget* parent) {
	setupUi(this);
	m_noEvent = 0;

	//Project
	m_project = new Project(this);
	view->setScene( m_project->scene() );
	view->createWidgets();
	view->setProject( m_project );

	//Export Dialog
	m_export = new Export(this);
	m_export->setProject(m_project);
	connect( actionExportFrame, 	SIGNAL( triggered() ), this, SLOT( exportFrame() ));
	connect( actionExportAnimation, SIGNAL( triggered() ), this, SLOT( exportAnimation() ));
	connect( actionExportAll, 	SIGNAL( triggered() ), this, SLOT( exportAll() ));
	connect( m_export,		SIGNAL( refreshCache(Animation*,bool)), view, SLOT( generateCache(Animation*,bool) ));

	//Undo
	m_commands = new CommandStack(this);
	m_commands->setActions( actionUndo, actionRedo );
	m_commands->setProject( m_project );
	connect( actionUndo, SIGNAL( triggered() ), m_commands, SLOT( undo() ));
	connect( actionRedo, SIGNAL( triggered() ), m_commands, SLOT( redo() ));
	connect( m_commands, SIGNAL( skipEvents(bool) ), this, SLOT( supressEvents(bool) ));
	connect( m_commands, SIGNAL( updateFrame(int) ), this, SLOT( refreshFrames() ));
	connect( m_commands, SIGNAL( updateTable() ), this, SLOT( refreshTable() ));
	connect( m_commands, SIGNAL( updateView() ), this, SLOT( setFrame() ));
	connect( m_commands, SIGNAL( updatePart(Part*, const Frame&) ), view, SLOT( updatePart(Part*, const Frame&) ));
	connect( m_commands, SIGNAL( updatePart(Part*, const Frame&) ), this, SLOT( updateDetails(Part*) ));
	view->setCommandStack( m_commands );

	//Debug
	connect( actionDebugUndo, SIGNAL( triggered() ), m_commands, SLOT(dumpStack() ));

	//Playback
	m_timer = new QTimer(this);
	connect(m_timer, SIGNAL( timeout() ), this, SLOT( step() ));
	connect(actionPlay, SIGNAL( triggered() ), this, SLOT( play() ));
	connect(playRate,   SIGNAL( valueChanged(double) ), this, SLOT( setRate(double) ));
	connect(btnPlay,    SIGNAL( clicked() ), actionPlay, SLOT( trigger() ));
	connect(btnLoop,    SIGNAL( clicked(bool) ), this, SLOT( setLoop(bool) ));
	//Add pause icon to button
	QIcon icon = btnPlay->icon();
	icon.addFile(":/icon/res/iconpause.png", QSize(), QIcon::Normal, QIcon::On);
	btnPlay->setIcon(icon);
	

	//Menu
	connect(actionNewProject,	SIGNAL(triggered()), this, SLOT(newProject()));
	connect(actionLoadProject,	SIGNAL(triggered()), this, SLOT(loadProject()));
	connect(actionSaveProject,	SIGNAL(triggered()), this, SLOT(saveProject()));
	connect(actionSaveProjectAs,	SIGNAL(triggered()), this, SLOT(saveProjectAs()));
	connect(actionImportXCF,	SIGNAL(triggered()), this, SLOT(importXCF()));
	
	//View Menu
	connect(actionZoomIn,	SIGNAL(triggered()), this, SLOT(zoomIn()));
	connect(actionZoomOut,	SIGNAL(triggered()), this, SLOT(zoomOut()));
	connect(actionResetZoom,SIGNAL(triggered()), this, SLOT(resetZoom()));

	//Panel visibility
	connect(animationsFrame, SIGNAL( visibilityChanged(bool) ), actionAnimations, SLOT( setChecked(bool) ));
	connect(actionAnimations, SIGNAL( triggered(bool) ), animationsFrame, SLOT( setVisible(bool) ));
	connect(partsFrame, SIGNAL( visibilityChanged(bool) ), actionParts, SLOT( setChecked(bool) ));
	connect(actionParts, SIGNAL( triggered(bool) ), partsFrame, SLOT( setVisible(bool) ));
	connect(detailsFrame, SIGNAL( visibilityChanged(bool) ), actionDetails, SLOT( setChecked(bool) ));
	connect(actionDetails, SIGNAL( triggered(bool) ), detailsFrame, SLOT( setVisible(bool) ));

	//Parts treeview
	QStandardItemModel* listModel = new QStandardItemModel();
	QItemSelectionModel* selectModel = new QItemSelectionModel( listModel );
	partsList->setModel( listModel );
	partsList->setSelectionModel(selectModel);

	//Parts events
	connect( m_project,		SIGNAL( changedPart(int)), this,	SLOT( updatePartList(int) ));
	connect( m_project,		SIGNAL( changedSelection(Part*)), this,	SLOT( updatePartSelection() ));
	connect( selectModel, SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(selectPart(const QModelIndex&,const QModelIndex&)));
	connect( listModel,		SIGNAL(itemChanged(QStandardItem*)), this, SLOT(renamePart(QStandardItem*)) );
	connect( btnAddPart,		SIGNAL( clicked() ), actionAddPart, 	SLOT( trigger() ));
	connect( btnDeletePart,		SIGNAL( clicked() ), actionDeletePart, 	SLOT( trigger() ));
	connect( btnEditParts,		SIGNAL( clicked() ), actionEditMode, 	SLOT( trigger() ));
	connect( actionAddPart,		SIGNAL( triggered() ), this, 		SLOT( addPart() ));
	connect( actionAddNull,		SIGNAL( triggered() ), this, 		SLOT( addNullPart() ));
	connect( actionClonePart,	SIGNAL( triggered() ), this, 		SLOT( clonePart() ));
	connect( actionDeletePart,	SIGNAL( triggered() ), this, 		SLOT( removePart() ));
	connect( actionMoveForward,	SIGNAL( triggered() ), view, 		SLOT( moveForward() ));
	connect( actionMoveBack,	SIGNAL( triggered() ), view, 		SLOT( moveBack() ));
	connect( actionEditMode,	SIGNAL( triggered(bool) ), this, 	SLOT( toggleEdit(bool)) );
	toggleEdit(false);

	//Animations list
	listModel = new QStandardItemModel();
	selectModel = new QItemSelectionModel( listModel );
	animationList->setModel( listModel );
	animationList->setSelectionModel(selectModel);
	connect( m_project,		SIGNAL( changedAnimation(int)), this,	SLOT( updateAnimationList(int) ));
	connect( m_project,		SIGNAL( changedSelection(Animation*)), this,	SLOT( updateAnimation() ));
	connect( selectModel,		SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(setAnimation(const QModelIndex&,const QModelIndex&)));
	connect( listModel,		SIGNAL(itemChanged(QStandardItem*)), this, SLOT(renameAnimation(QStandardItem*)) );
	connect( btnAddAnimation,	SIGNAL( clicked() ), actionAddAnimation, SLOT( trigger() ));
	connect( btnCopyAnimation,	SIGNAL( clicked() ), actionDuplicateAnimation, SLOT( trigger() ));
	connect( btnDeleteAnimation,	SIGNAL( clicked() ), actionDeleteAnimation, SLOT( trigger() ));
	connect( actionAddAnimation,	SIGNAL( triggered() ), this, SLOT( addAnimation() ));
	connect( actionDuplicateAnimation, SIGNAL(triggered()), this,SLOT( cloneAnimation() ));
	connect( actionDeleteAnimation, SIGNAL( triggered() ), this, SLOT( deleteAnimation() ));
	connect( btnMoveUp,		SIGNAL( clicked() ), actionMoveUp,   SLOT( trigger() ));
	connect( btnMoveDown,		SIGNAL( clicked() ), actionMoveDown, SLOT( trigger() ));
	connect( actionMoveUp,		SIGNAL( triggered()), this, SLOT( moveAnimationUp() ));
	connect( actionMoveDown,	SIGNAL( triggered()), this, SLOT( moveAnimationDown() ));

	//Animations - how to do this??
	m_frameModel = new TableModel();
	m_frameModel->setProject( m_project );
	TableDelegate* delegate = new TableDelegate(this);
	selectModel = new QItemSelectionModel( m_frameModel );
	frameList->setModel(m_frameModel);
	frameList->setItemDelegate( delegate );
	frameList->horizontalHeader()->setMinimumSectionSize(4);
	frameList->verticalHeader()->setMinimumSectionSize(4);
	frameList->setSelectionModel(selectModel);

	//Frame controls
	connect( selectModel,    SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(selectFrame(const QModelIndex&,const QModelIndex&)));
	connect( frameCount,     SIGNAL( valueChanged(int) ),	this, SLOT( setFrameCount(int) ));
	connect( btnInsertFrame, SIGNAL( clicked() ),		this, SLOT( insertFrame() ));
	connect( btnDeleteFrame, SIGNAL( clicked() ),		this, SLOT( deleteFrame() ));
	connect( autoKey,	 SIGNAL( clicked() ), actionAutoKey,  SLOT( trigger() ));
	connect( actionAutoKey,	 SIGNAL( triggered(bool) ),	view, SLOT( setAutoKey(bool) ));

	//Onion Skin
	connect( onionBefore,	    SIGNAL( clicked() ), actionOnionBefore, SLOT( trigger() ));
	connect( onionAfter,	    SIGNAL( clicked() ), actionOnionAfter,  SLOT( trigger() ));
	connect( actionOnionBefore, SIGNAL( triggered(bool) ), onionBefore, SLOT( setChecked(bool) ));
	connect( actionOnionAfter,  SIGNAL( triggered(bool) ), onionAfter,  SLOT( setChecked(bool) ));
	connect( actionOnionBefore, SIGNAL( triggered(bool) ), this,        SLOT( updateOnionSkin() ));
	connect( actionOnionAfter,  SIGNAL( triggered(bool) ), this,        SLOT( updateOnionSkin() ));
	connect( onionSize,	    SIGNAL( valueChanged(int) ), this,      SLOT( updateOnionSkin() ));
	
	//Details
	connect( frameAngle,	SIGNAL( toggled(bool) ),	this, SLOT( changeFrameMode() ));
	connect( frameOffset,	SIGNAL( toggled(bool) ),	this, SLOT( changeFrameMode() ));
	connect( frameVisible,	SIGNAL( toggled(bool) ),	this, SLOT( changeFrameMode() ));
	connect( angleValue,	SIGNAL( valueChanged(double) ), this, SLOT( changeAngleValue(double) ));
	connect( offsetX,	SIGNAL( valueChanged(double) ), this, SLOT( changeOffsetValue() ));
	connect( offsetY,	SIGNAL( valueChanged(double) ), this, SLOT( changeOffsetValue() ));
	connect( frameHidden,	SIGNAL( toggled(bool) ),	this, SLOT( changeHiddenValue(bool) ));

	connect( pivotX,	SIGNAL( valueChanged(double) ), this, SLOT( changePivotValue() ));
	connect( pivotY,	SIGNAL( valueChanged(double) ), this, SLOT( changePivotValue() ));
	connect( positionX,	SIGNAL( valueChanged(double) ), this, SLOT( changeRestValue() ));
	connect( positionY,	SIGNAL( valueChanged(double) ), this, SLOT( changeRestValue() ));
	connect( partHidden,	SIGNAL( toggled(bool) ),	this, SLOT( changeRestHidden(bool) ));

	//connect( view, SIGNAL( partChanged(Part*) ), this, SLOT( editPart(Part*) ));
	connect( actionNextFrame,	SIGNAL( triggered() ), this, SLOT( nextFrame() ));
	connect( actionPreviousFrame,	SIGNAL( triggered() ), this, SLOT( previousFrame() ));

	
}

void AnimTool::nextFrame() {
	Animation* anim = m_project->currentAnimation();
	if(anim && m_project->frame() < anim->frameCount()-1) {
		setFrame( m_project->frame()+1 );
	} else setFrame(0);
}
void AnimTool::previousFrame() {
	if(m_project->frame() >0) {
		setFrame( m_project->frame()-1 );
	} else if(m_project->currentAnimation()) {
		setFrame( m_project->currentAnimation()->frameCount()-1);
	}
}

void AnimTool::zoomIn() { view->scale(1.1,1.1); }
void AnimTool::zoomOut() { view->scale(0.9,0.9); }
void AnimTool::resetZoom() { view->resetTransform(); }

//// //// //// //// //// //// //// //// Utility functions  //// //// //// //// //// //// //// //// 

void AnimTool::supressEvents( bool s ) {
	if(s) m_noEvent++;
	else if(m_noEvent) m_noEvent--;
}

QStandardItem* AnimTool::findItem(QAbstractItemModel* amodel, int data) {
	return findItem(amodel, QString::null, data);
}
QStandardItem* AnimTool::findItem(QAbstractItemModel* amodel, const QString& text, int data) {
	QStandardItemModel* model = static_cast<QStandardItemModel*>(amodel);
	QList<QStandardItem*> items;
	if(text==QString::null) items = model->findItems( "*", Qt::MatchRecursive | Qt::MatchWildcard );
	else items = model->findItems( text, Qt::MatchRecursive );
	for(int i=0; i<items.size(); i++) if(data<0 || items[i]->data().toInt() == data) return items[i];
	return 0;
}

//// //// //// //// //// //// //// //// File Commands //// //// //// //// //// //// //// ////

bool AnimTool::confirmClose() {
	if(m_commands->isClean()) return true; //Clean project - no need to ask
	QMessageBox msg;
	msg.setText("This project has been modified");
	msg.setInformativeText("Do you want to save your changes?");
	msg.setStandardButtons( QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
	msg.setDefaultButton(QMessageBox::Save);
	int r = msg.exec();
	if(r==QMessageBox::Save) saveProject();
	return r!=QMessageBox::Cancel;
}
void AnimTool::clearProject() {
	m_project->clear();
	view->selectItem(0);
	m_frameModel->setAnimation(0);
	setAnimation(QModelIndex(), QModelIndex());
	resetZoom();
	m_commands->clear();
	m_commands->setClean();
}
void AnimTool::newProject() {
	if(confirmClose()) clearProject();
}
void AnimTool::loadProject() {
	if( !confirmClose() ) return; //Cancel
	QString file = QFileDialog::getOpenFileName( this, "Load Project", QString::null, "Animation Project (*.anim)" );
	if(file != QString::null) {
		clearProject();
		m_project->loadProject(file);
		m_commands->setClean();
	}
}
void AnimTool::saveProject() {
	if(m_project->getFile() == QString::null) saveProjectAs();
	else {
		m_project->saveProject( m_project->getFile() );
		m_commands->setClean();
		statusbar->showMessage("Project Saved", 3000);
	}
}
void AnimTool::saveProjectAs() {
	QString file = QFileDialog::getSaveFileName( this, "Save Project", QString::null, "Animation Project (*.anim)" );
	if(file!=QString::null) {
		m_project->saveProject(file);
		m_commands->setClean();
		statusbar->showMessage("Project Saved", 3000);
	}
}

//// //// //// //// //// //// //// //// Exporting //// //// //// //// //// //// //// ////

void AnimTool::exportFrame() {
	m_export->exportFrame( m_project->currentAnimation(), m_project->frame());
}
void AnimTool::exportAnimation() {
	QList<Animation*> list;
	list.push_back( m_project->currentAnimation() );
	m_export->setAnimations( list );
	m_export->open();
}
void AnimTool::exportAll() {
	m_export->setAnimations( m_project->animations() );
	m_export->open();
}

//// //// //// //// //// //// //// //// Parts List //// //// //// //// //// //// //// ////

void AnimTool::updatePartList(int id) {
	Part* part = m_project->getPart(id);
	//Can be Deleted or Added
	if(id==0) {			// -------- Rebuild tree ---------
		static_cast<QStandardItemModel*>(partsList->model())->clear();
		printf("ToDo: Rebuild part tree view\n");
	} else if(id && !part) {	// --------- Delete item ---------
		QStandardItem* item = findItem(partsList->model(), id);
		if(item->parent()) item->parent()->removeRow( item->row() );
		else partsList->model()->removeRow( item->row() );
	} else {			// --------- Add item ------------
		QStandardItem* item = new QStandardItem( part->getName() );
		item->setData( part->getID() );
		//Get parent and add to list
		QStandardItem* parent = 0;
		if(part->getParent()) parent = findItem(partsList->model(), part->getParent()->getName(), part->getParent()->getID());
		if(!parent) parent = static_cast<QStandardItemModel*>( partsList->model() )->invisibleRootItem();
		parent->appendRow( item );
		//Set formatting
		if(part->pixmap().isNull()) {
			QFont font = item->font(); font.setItalic(true);
			item->setFont( font );
		}
		printf("Add %d\n", item->data().toInt());
	}
}

void AnimTool::updatePartSelection() {
	Part* part = m_project->currentPart();
	//Select part in treeView
	supressEvents(true);
	if(part) {
		QStandardItem* item = findItem(partsList->model(), part->getName(), part->getID());
		partsList->setCurrentIndex( item->index() );
	} else {
		partsList->setCurrentIndex( QModelIndex() ); //deselect
	}
	supressEvents(false);
	// Enable / disable widgets
	bool enabled = part;
	btnDeletePart		-> setEnabled( enabled );
	actionDeletePart	-> setEnabled( enabled );
	actionClonePart		-> setEnabled( enabled );
	actionCloneHeirachy	-> setEnabled( enabled );
	actionMoveForward	-> setEnabled( enabled );
	actionMoveBack		-> setEnabled( enabled );
	partDetails			-> setEnabled( enabled );
	frameDetails		-> setEnabled( enabled && m_project->currentAnimation() );
	partPivot			-> setEnabled( enabled && !part->pixmap().isNull() );
	updateDetails( part );
	refreshFrames();
	//Select in view
	view->selectItem( part );
}
void AnimTool::selectPart(const QModelIndex& sel, const QModelIndex& last) {
	if(m_noEvent) return;
	int part = sel.isValid()? sel.data(Qt::UserRole+1).toInt(): 0;
	m_project->select( m_project->getPart( part ) );
}
void AnimTool::renamePart(QStandardItem* item) {
	printf("MovePart %d\n", item->data().toInt());
	if(m_noEvent) return;
	Part* part = m_project->getPart( item->data().toInt());
	assert(part && " Failed to find part " );
	//Rename command?
	if(item->text() != part->getName()) {
		m_commands->push(  new RenamePart(part, item->text(), partsList) );
	}
	//Change parent command?
	Part* parent = item->parent()? m_project->getPart( item->parent()->data().toInt() ): 0;
	if(parent != part->getParent()) {
		part->setParent(parent); // Here as we are not executing the command HACK
		m_commands->push( new MovePart(part, parent, partsList), false );
	}
}
void AnimTool::addPart() {
	//Open file dialogue to open image file
	QString f;
	QList<QByteArray> formats = QImageReader::supportedImageFormats();
	for(QList<QByteArray>::Iterator i=formats.begin(); i!=formats.end(); i++) f += " *." + *i;
	QStringList files = QFileDialog::getOpenFileNames( this, "Load Part Image", QString::null, "Images (" + f + ")" );
	for(QStringList::Iterator i=files.begin(); i!=files.end(); ++i) {
		printf("Load part %s\n", i->toAscii().data());
		QString name = i->section('/',-1);
		// Create command
		m_commands->push( new CreatePart(name, *i, m_project->currentPart()) );
	}
}

void AnimTool::addNullPart() {
	m_commands->push( new CreatePart("null", QString::null, m_project->currentPart()) );
}

void AnimTool::clonePart() {
	Part* old = m_project->currentPart();
	if(old) m_commands->push( new ClonePart(old) );
}

void AnimTool::importXCF() {
	QString file = QFileDialog::getOpenFileName( this, "Import Parts from Image", QString::null, "XCF Images (*.xcf)" );
	if(file!=QString::null) m_project->importXCF(file);
}

void AnimTool::removePart() {
	//remove selected item
	if(m_project->currentPart()) {
		m_commands->push( new DeletePart( m_project->currentPart() ) );
	}
}

void AnimTool::toggleEdit(bool edit) {
	view->setMode(edit);
	btnEditParts->setChecked(edit);
	if(edit) { frameDetails->hide(); partDetails->show(); }
	else { partDetails->hide(); frameDetails->show(); }
}

//// //// //// //// //// //// //// //// Animation List //// //// //// //// //// //// //// ////

void AnimTool::updateAnimation() {
	Animation* anim = m_project->currentAnimation();

	//Stop playback
	if(m_timer->isActive()) play();

	//Select animation in list
	supressEvents(true);
	if(anim) {
		QStandardItem* item = findItem(animationList->model(), anim->name(), anim->getID());
		animationList->setCurrentIndex( item->index() );
	} else animationList->setCurrentIndex( QModelIndex() );
	
	//Enable / Disable widgets
	btnCopyAnimation	-> setEnabled( anim );
	btnDeleteAnimation	-> setEnabled( anim );
	actionDuplicateAnimation-> setEnabled( anim );
	actionDeleteAnimation	-> setEnabled( anim );
	frameDataContents	-> setEnabled( anim );
	frameDetails		-> setEnabled( anim && m_project->currentPart() );
	actionExportAnimation	-> setEnabled( anim );
	actionExportFrame	-> setEnabled( anim );
	
	//Animation frames
	static_cast<TableModel*>(frameList->model()) -> setAnimation( anim );
	playRate	-> setValue( anim? anim->frameRate(): 15.0f );
	btnLoop		-> setChecked( anim? anim->loop(): true );
	frameCount	-> setValue( anim? anim->frameCount(): 0);
	refreshTable();

	//Set current frame
	int f = anim? m_project->frame() : 0;
	if(anim && f >= anim->frameCount()) f = anim->frameCount()-1;
	setFrame(f); //Update view
	supressEvents(false);
}

void AnimTool::updateAnimationList(int id) {
	Animation* anim = m_project->getAnimation(id);
	supressEvents(true);
	QStandardItemModel* model = static_cast<QStandardItemModel*>(animationList->model());
	QStandardItem* item;
	if(id==0) { 		// -------- Refresh entire list ------------
		QList<Animation*> items = m_project->animations();
		model->clear();
		for(int i=0; i<items.size(); i++) {
			item = new QStandardItem( items[i]->name() );
			item->setData(items[i]->getID());
			model->appendRow( item );
		}
		model->sort(0); // Sort list?
	} else if(!anim) { 	// --------- Delete element ----------------
		item = findItem(animationList->model(), id);
		if(item) animationList->model()->removeRow( item->row() );
	} else { 		// ----------   Add Item -------------------
		item = new QStandardItem(anim->name());
		item->setData( id );
		static_cast<QStandardItemModel*>(animationList->model())->appendRow( item );
		model->sort(0); // Sort list?
	}
	actionExportAll -> setEnabled( model->rowCount() );
	supressEvents(false);
}

void AnimTool::addAnimation() {
	m_commands->push( new AddAnimation("New Animation") );
}
void AnimTool::cloneAnimation() {
	m_commands->push( new CloneAnimation( m_project->currentAnimation() ) );
}
void AnimTool::deleteAnimation() {
	m_commands->push( new DeleteAnimation( m_project->currentAnimation() ) );
}
void AnimTool::setAnimation(const QModelIndex& sel, const QModelIndex& last) {
	if(m_noEvent) return;
	Animation* anim = 0;
	if(sel.isValid()) anim = m_project->getAnimation( sel.data(Qt::UserRole+1).toInt() );
	m_project->setCurrent( anim );
}
void AnimTool::renameAnimation(QStandardItem* item) {
	if(m_noEvent) return;
	//Get animation - assume we can only rename the current animation
	Animation* anim = m_project->getAnimation( item->data().toInt() );
	if(item->text()=="") item->setText( anim->name() );
	else if(item->text()!=anim->name()) {
		m_commands->push( new RenameAnimation(anim, item->text(), animationList) );
	}
	//Was it a move event? TODO
}
void AnimTool::moveAnimationDown() {
	int index = animationList->currentIndex().isValid()? animationList->currentIndex().row(): -1;
	m_project->moveAnimation( m_project->currentAnimation(), index+1);

}
void AnimTool::moveAnimationUp() {
	int index = animationList->currentIndex().isValid()? animationList->currentIndex().row(): -1;
	m_project->moveAnimation( m_project->currentAnimation(), index-1);
}

//// //// //// //// //// //// //// //// Frame Buttons //// //// //// //// //// //// //// ////

void AnimTool::setFrameCount(int c) {
	Animation* anim = m_project->currentAnimation();
	if(anim && anim->frameCount()!=c) m_commands->push( new SetFrames( anim, c ) );
}
void AnimTool::insertFrame() {
	Animation* anim = m_project->currentAnimation();
	m_commands->push( new InsertFrame(anim, m_project->frame()) );
}
void AnimTool::deleteFrame() {
	Animation* anim = m_project->currentAnimation();
	m_commands->push( new DeleteFrame(anim, m_project->frame()) );
}

//// //// //// //// //// //// //// //// Details panel //// //// //// //// //// //// //// ////

//// Update data from Part values ////
void AnimTool::updateDetails(Part* part, int mask) {
	if(!part) return; // TODO Perhaps clear values?
	supressEvents(true);
	if(mask&1) { // Frame values
		float    angle  = part->localAngle();
		QPointF  offset = part->localOffset();
		angleValue  -> setValue( angle );
		offsetX     -> setValue( offset.x() );
		offsetY     -> setValue( offset.y() );
		frameHidden -> setChecked( !part->isVisible() );
	}
	if(mask&2) { // Rest Values
		pivotX      -> setValue( -part->offset().x() );
		pivotY      -> setValue( -part->offset().y() );
		positionX   -> setValue( part->rest().x() );
		positionY   -> setValue( part->rest().y() ); 
		partHidden  -> setChecked( part->hidden() );
	}
	if(mask&4) {// Keyframes?
		Animation* anim = m_project->currentAnimation();
		int mode = anim? anim->isKeyframe( m_project->frame(), part) : 0;
		frameAngle  -> setChecked( mode&1 );
		frameOffset -> setChecked( mode&2 );
		frameHidden -> setChecked( mode&4 );
	}
	supressEvents(false);
}

//// Frame ////
void AnimTool::changeFrameMode() {
	if(m_noEvent) return;
	int mode = (frameAngle->isChecked()?1:0) | (frameOffset->isChecked()?2:0) | (frameVisible->isChecked()?4:0);
	Animation* anim = m_project->currentAnimation();
	Part* part = m_project->currentPart();
	int frame = m_project->frame();
	int oldKey = anim?anim->isKeyframe(frame, part): 0;
	m_commands->push( new ChangeFrameData(anim, part, frame, oldKey, mode) );
}
void AnimTool::changeOffsetValue() {
	if(m_noEvent) return;
	Animation* anim = m_project->currentAnimation();
	Part*      part = m_project->currentPart();
	int       frame = m_project->frame();
	QPointF offset( offsetX->value(), offsetY->value() );
	m_commands->push( new ChangeFrameData( anim, part, frame, part->localOffset(), offset) );
}
void AnimTool::changeAngleValue(double value) {
	if(m_noEvent) return;
	Animation* anim = m_project->currentAnimation();
	Part*      part = m_project->currentPart();
	int       frame = m_project->frame();
	m_commands->push( new ChangeFrameData( anim, part, frame, part->localAngle(), value) );
}
void AnimTool::changeHiddenValue(bool hidden) {
	if(m_noEvent) return;
	Animation* anim = m_project->currentAnimation();
	Part*      part = m_project->currentPart();
	int       frame = m_project->frame();
	m_commands->push( new ChangeFrameData( anim, part, frame, part->isVisible(), !hidden) );
}
//// Rest ////
void AnimTool::changePivotValue() {
	if(m_noEvent) return;
	Part* part = m_project->currentPart();
	QPointF pivot( -pivotX->value(), -pivotY->value() );
	m_commands->push( new ChangeRest(part, pivot, 1) );
}
void AnimTool::changeRestValue() {
	if(m_noEvent) return;
	Part* part = m_project->currentPart();
	QPointF rest( positionX->value(), positionY->value() );
	m_commands->push( new ChangeRest(part, rest, 2) );
}
void AnimTool::changeRestHidden(bool hidden) {
	if(m_noEvent) return;
	Part* part = m_project->currentPart();
	m_commands->push( new ChangeRest(part, hidden) );
}

//// //// //// //// //// //// //// //// Frames //// //// //// //// //// //// //// ////

void AnimTool::selectFrame(const QModelIndex& sel, const QModelIndex& last) {
	if(sel.isValid()) setFrame( sel.column() );
}
void AnimTool::setFrame(int frame) {
	if(frame<0) frame = m_project->frame(); //Re-apply the current frame
	Animation* anim = m_project->currentAnimation();
	if(anim) {
		m_project->setFrame( frame );
		updateDetails( m_project->currentPart() );
		//Update view
		int before = onionBefore->isChecked()? onionSize->value(): 0;
		int after  = onionAfter ->isChecked()? onionSize->value(): 0;
		view->displayFrame( anim, frame, before, after );
	} else view->displayFrame(0,0);
	refreshFrames();
}
void AnimTool::refreshFrames(int f) {
	for(int i=0; i<m_frameModel->rowCount(); i++) {
		if(f>=0) frameList->update( m_frameModel->index(i,f) );
		else for(int j=0; j<m_frameModel->columnCount(); j++) {
			frameList->update( m_frameModel->index(i,j) );
		}
	}
}
void AnimTool::refreshTable() {
	Animation* anim = m_project->currentAnimation();
	m_frameModel->flagChange();
	frameList->resizeColumnsToContents();
	frameList->resizeRowsToContents();
	refreshFrames(); //probably unnessesary
	//Refresh framecount box
	supressEvents(true);
	frameCount->setValue( anim? anim->frameCount(): 0 );
	supressEvents(false);
}

//// //// //// //// //// //// //// //// Onion Skin //// //// //// //// //// //// //// ////

void AnimTool::updateOnionSkin() {
	int before = onionBefore->isChecked()? onionSize->value(): 0;
	int after  = onionAfter ->isChecked()? onionSize->value(): 0;
	printf("Set onionskin %d-%d\n", before, after);
	view->setOnionSkin(before, after);
}

//// //// //// //// //// //// //// //// Playback //// //// //// //// //// //// //// ////

void AnimTool::play(bool go) {
	if(!go || m_timer->isActive()) { //Stop playback
		m_timer->stop();
	} else if(m_project->currentAnimation()->frameCount()>1) {
		nextFrame();
		m_timer->start( 1000 / playRate->value() );
	}
	btnPlay->setChecked( m_timer->isActive() );
}

void AnimTool::setLoop(bool loop) {
	m_project->currentAnimation()->setLoop(loop);
	setFrame( m_project->frame() ); //may have changed
}

void AnimTool::setRate(double fps) {
	printf("Adjust %f\n", 1000/fps);
	if(m_timer->isActive()) m_timer->setInterval( 1000 / fps );
	if(m_project->currentAnimation()) m_project->currentAnimation()->setFrameRate( fps );
}

void AnimTool::step() {
	Animation* anim = m_project->currentAnimation();
	if(!anim->loop() && m_project->frame() == anim->frameCount()-1) { play(false); return; }
	nextFrame();
}



