#ifndef _PARTS_
#define _PARTS_

#include <QGraphicsPixmapItem>
#include <cstdio>

class Part : public QGraphicsPixmapItem {
	public:
	Part(int id, bool null=false) : m_id(id), m_isNull(null), m_parent(0), m_hidden(0) {}
	int getID() const { return m_id; }				// Get part unique id
	void setImage(const QPixmap& img) { setPixmap(img); }		// Set part graphic
	void setName(const QString& s) { m_name = s; }			// Set part name
	const QString& getName() const { return m_name; }		// Get part name
	Part* getParent() { return m_parent; }				// Get parent part
	void setParent(Part* parent) {					// Attach to parent part
		if(m_parent) {
			m_parent->m_children.removeAll(this);
			m_rest = absoluteRest();
		}
		if(parent) {
			parent->m_children.push_back(this);
			m_rest -= parent->absoluteRest();
		}
		m_parent = parent;
	}
	QList<Part*>& children() { return m_children; }			// Get list of children
	bool isNull() const { return m_isNull; }				// Is this part a null marker

	//Base data
	void setRest(const QPointF& p) { m_rest=p; }			// Set relative rest position
	const QPointF& rest() const { return m_rest; }			// Get relative rest position
	void setHidden(bool h) { m_hidden=h; }				// Set hidden by default
	bool hidden() { return m_hidden; }				// Is the part hidden by default

	void setSource(const QString& f) { m_source=f; }		// Set source file name data
	const QString& getSource() const { return m_source; }		// Get source file name data

	//Calculate Frame data
	QPointF localOffset() const { return (m_parent? m_parent->mapFromItem(0,pos()): pos()) - m_rest; }
	float   localAngle() const { return m_parent? rotation() - m_parent->rotation(): rotation(); }

	protected:
	int          m_id;			// Part ID
	bool         m_isNull;		// Null part?
	QString      m_name;		// Part name
	QString      m_source;		// Source image
	Part*        m_parent;		// Parent part
	QList<Part*> m_children;	// Child parts
	bool         m_hidden;		// Hidden by default
	QPointF      m_rest;		// Rest position

	QPointF absoluteRest() { return m_parent? m_parent->absoluteRest()+m_rest: m_rest; }
};

typedef QList<Part*> PartList;

#endif

