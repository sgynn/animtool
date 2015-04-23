#ifndef _VIEW_
#define _VIEW_

#include <QGraphicsView>
#include <QMouseEvent>
#include <QWheelEvent>

#include "animation.h"

class Project;
class Part;
class Command;
class CommandStack;

class View : public QGraphicsView {
	Q_OBJECT;
	public:
	View(QWidget* parent=0);

	void setProject(Project* p) { m_project=p; }				//Set the current project
	void createWidgets();							//Create info widgets
	void setCommandStack(CommandStack* stack);				//Set the command stack

	Part* partAt(const QPointF&);						//Get a part at a point (for selection)

	public slots:
	void moveForward() { moveZ(-1); }					//Move selected part up
	void moveBack() { moveZ(1); }						//Move selected part down
	void moveZ(int z);							//Move up or down
	void setMode(bool edit);						//Set edit mode
	void selectItem(Part* item);						//Set the selected item
	void setAutoKey(bool on) { m_autoKey=on; }				//Set autokey mode

	void displayFrame(Animation* anim, int frame, int before=0,int after=0);//Display a frame
	void updatePart(Part* part, const Frame& data);				//Update a part to framedata
	void updateControllers();									//Update all active controllers
	void setOnionSkin(int before=0, int after=0);				//Change the onion skin
	void generateCache(Animation* anim, bool override=false);		//Generate cached frame images

	void setAbsoluteRotation(Part* part, float degrees);			// Set part rotation, used by controller

	protected:
	Project* m_project;							//project
	bool m_edit;								//Edit rest data

	QGraphicsEllipseItem* m_pivot;						//Pivot widget
	QGraphicsLineItem* m_pivotLine[2];					//Lines in the pivot widget
	QGraphicsLineItem* m_parentLine;					//Parent Line widget
	QGraphicsPathItem* m_outline;						//Selected part outline
	Part* m_selected;							//Selected part

	int m_mode; 	//1=move, 2=rotate					//Edit mode
	QPointF m_moveOffset;							//Mouse offset for dragging
	float m_angleOffset;							//Mouse offset for rotating
	bool m_moved; 								//Mouse moved - for selecting stuff in view
	bool m_autoKey;								//Automatically add keyframes

	Animation* m_animation;							//Current Animation
	int m_frame;								//Current Frame
	bool m_frameChanged;							//Has the current frame been modified

	QList<QGraphicsPixmapItem*> m_onion;					//The onion skin
	unsigned int m_lastOnion;						//Last state of the onion skin

	void updateAll(Animation*, int frame);					//Update all parts to animation data
	void updateSelection();							//Update selection widgets
	void setPivot(Part* part, const QPointF& point);			//Set the pivot to scene coordinates
	void setRest(Part* part, const QPointF& point);				//Set the rest position to scene coordinates
	void moveRest(Part* part, const QPointF& move);				//Move the rest position in scene coordinates
	void rotatePart(Part* part, float angle);				//Rotate a part by angle
	void movePart(Part* part, const QPointF& move);				//Move a part by move
	void rotateChildren(Part* part, const QPointF& pivot, float angle);	//Rotate all child parts by angle
	void moveChildren(Part* part, const QPointF& offset);			//Move all child objects by offset

	QPointF toParent(Part* part, const QPointF&) const;			//Map point to parent part's coordinates

	void drawBackground(QPainter* painter, const QRectF& rect);		//Draw background
	void cacheFrame(Animation* anim, int frame);				//Cache animation frame for onion skinning / exporting
	QPixmap fadeImage(const QPixmap& src, float alpha);			//Set the alpha value of an image

	void mouseMoveEvent(QMouseEvent*);					//Mouse events
	void mousePressEvent(QMouseEvent*);
	void mouseReleaseEvent(QMouseEvent*);
	void wheelEvent(QWheelEvent*);

	CommandStack* m_commands;
	void pushCommand(Command* cmd, bool execute=true);			//Push a command to stack


};

#endif

