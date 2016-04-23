#ifndef _QT_TEST_
#define _QT_TEST_


#include "ui_anim.h" //The generated form from anim.ui

#include "project.h"
#include "tablemodel.h"

class Export;
class CommandStack;
class QStandardItem;
class QAbstractItemModel;

class AnimTool : public QMainWindow, private Ui::MainWindow {
	Q_OBJECT; //Random QT Macro for the signals/slots stuff
	public:
	AnimTool(QWidget* parent=0);
	bool confirmClose();
	void clearProject();

	// Utility
	static QStandardItem* findItem(QAbstractItemModel* model, const QString& name, int data=-1);
	static QStandardItem* findItem(QAbstractItemModel* model, int data);

	private:

	Project* m_project;
	TableModel* m_frameModel;

	int m_noEvent;			// Block events to change multiple spinboxes at once
	QTimer* m_timer;		// Timer for playback
	CommandStack* m_commands;	// Command stack
	Export* m_export;		// Export Dialog

	public slots:
	
	void supressEvents(bool=true);
	void updateTitle();
	
	void newProject();
	void loadProject();
	void saveProject();
	void saveProjectAs();
	void importXCF();

	void exportFrame();
	void exportAnimation();
	void exportAll();

	void copyFrameData();	// Clipboard actions
	void pasteFrameData();

	void updatePartList(int id);	// add/delete part from list
	void updatePartSelection();	// Change what is selected
	void addPart();
	void addNullPart();
	void clonePart();
	void cloneHeirachy();
	void removePart();
	void renamePart(QStandardItem*);
	void selectPart(const QModelIndex&, const QModelIndex&);

	void controllerSelected();
	void updateControllerList(int);
	void updateControllerParts();
	void addController();
	void removeController();
	void fillControllerParts(QComboBox*, Part*, int, bool);
	void controllerStateChanged(QListWidgetItem*);
	void validateController();

	void updateAnimation();
	void updateAnimationList(int id);
	void addAnimation();
	void cloneAnimation();
	void deleteAnimation();
	void renameAnimation(QStandardItem*);
	void setAnimation(const QModelIndex&, const QModelIndex&);
	void moveAnimationUp();
	void moveAnimationDown();

	void updateOnionSkin();

	void selectFrame(const QModelIndex&, const QModelIndex&);
	void setFrameCount(int);
	void insertFrame();
	void deleteFrame();

	void updateDetails(Part* part=0, int mask=7);
	void changeFrameMode();
	void changeOffsetValue();
	void changeAngleValue(double);
	void changeHiddenValue(bool);
	void changePivotValue();
	void changeRestValue();
	void changeRestHidden(bool);

	void nextFrame();
	void previousFrame();
	void setFrame(int frame=-1);
	void refreshFrames(int frame=-1);
	void refreshTable();

	void zoomIn();
	void zoomOut();
	void resetZoom();

	void play(bool go=true);
	void setLoop(bool loop);
	void setRate(double fps);
	void step();

	
};

#endif
