#ifndef _ANIMATION_TABLE_MODEL_
#define _ANIMATION_TABLE_MODEL_

#include <QAbstractTableModel>
#include <QAbstractItemDelegate>

class QPainter;

class Project;
class Animation;
class Part;

class TableModel : public QAbstractTableModel {
	Q_OBJECT; // qt events macro

	public:
	TableModel(QObject* parent=0);

	void setProject(Project* p) { m_project=p; }
	void setAnimation(Animation* anim);
	Animation* animation() const { return m_animation; }

	int rowCount(const QModelIndex& parent = QModelIndex()) const;
	int columnCount(const QModelIndex& parent = QModelIndex()) const;
	QVariant headerData(int, Qt::Orientation, int role=Qt::DisplayRole) const;
	QVariant data(const QModelIndex&, int role=Qt::DisplayRole) const;

	public slots:
	void flagChange() { reset(); }

	protected:
	Project* m_project;
	Animation* m_animation;
	int m_mode; // Selected Part, Selected+Children, All parts
};

class TableDelegate : public QAbstractItemDelegate {
	Q_OBJECT;
	public:
	TableDelegate(QObject* parent=0);
	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
};

#endif

