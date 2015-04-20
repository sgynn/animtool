#ifndef _PROJECT_
#define _PROJECT_

#include <QGraphicsScene>
#include <QString>
#include <QMap>

class QDomNode;
class Animation;
class Part;
class XCF;

class Project : public QObject {
	Q_OBJECT;
	public:
	Project(QObject* parent=0);
	~Project();
	void clear();						//Delete everything


	Part* getPart(int id);					// Get a part object by its ID
	Part* createPart(const QString& name, int id=0);	// Create a new part (id: 0=auto)
	void  addPart(Part* part, Part* parent=0);		// Add a part to the project
	void  removePart(Part* part);				// Remove a part from the project

	void select(Part* part);				// Select a part
	void select(QList<Part*> parts);			// Select a list of parts
	void deselect(Part* part);				// Deselect a part
	void deselect();					// Clear selection
	QList<Part*> selectedParts();				// Get selected parts
	Part* currentPart();					// Get the current part
	const QList<Part*> parts();					// Get all parts in an order where parent<child

	void addAnimation(Animation* anim, int id=0);		// Add an animation
	void removeAnimation(Animation* anim);			// Remove an animation
	void moveAnimation(Animation* anim, int index);		// Change the order of animations
	void setCurrent(Animation* anim);			// Set the current animation
	Animation* getAnimation(int id);			// Get animation by id
	Animation* currentAnimation();				// Get the current animation
	const QList<Animation*>& animations();			// Get all animations


	bool saveProject(const QString& file);			// Save project xml
	bool loadProject(const QString& file);			// load from xml
	QPixmap loadGraphic(const QString& file);		// Load an image - supports loading individual layers from xcf images
	int importXCF(const QString& file);			// Import XCF layers as parts

	QGraphicsScene* scene() { return &m_scene; }		// The graphical scene
	const QString& getFile() const { return m_file; }	// Get the project filename

	int frame() const { return m_currentFrame; }		// The current frame
	void setFrame(int f) { m_currentFrame=f; }		// Set the current frame

	signals:
	void changedPart(int id);				// Signal to update parts list view
	void changedAnimation(int);				// Signal to change animations list view
	void changedSelection(Part* part);			// Signal to change parts selection
	void changedSelection(Animation* anim);			// Signal to change current animation

	protected:
	QString m_file;						// Project filename

	QList<Animation*> m_animations;				// Animation list
	QMap<int, Part*> m_parts;				// Parts list
	QGraphicsScene m_scene;					// The scene
	int m_partValue;					// Value to create new part id's
	int m_animValue;					// Value to create animation ID's

	Part* m_currentPart;					// Current part
	QList<Part*> m_selection;				// Selected parts (includes current part)
	Animation* m_currentAnimation;				// Selected animation
	int m_currentFrame;					// Current frame

	// File utility functions
	QPointF readPoint(const QDomNode& node) const;		// Parse a point
	int readPart(QDomNode* node, int parent=0);		// Recursively read nodes from xml
	int writePart(QDomNode* node, Part* part);		// Recursively wrkite nodes to xml

	// XCF Cache
	QMap<QString, XCF*> m_xcfImages;
	QPixmap makeImage(const unsigned char* data, int w, int h, int bpp);

};

#endif

