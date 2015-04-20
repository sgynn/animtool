#include "project.h"
#include <QtXml>
#include <QFile>
#include <cstdio>

#include "part.h"
#include "animation.h"

#include "xcf.h"


bool Project::loadProject(const QString& filename) {
	printf("Loading project %s\n", filename.toAscii().data());
	QDomDocument doc;
	QFile file( filename);
	if(!file.open(QIODevice::ReadOnly)) return false;
	if (!doc.setContent(&file)) {
		file.close();
		return false;
	}
	file.close();

	//Read document ...
	m_file = filename;
	QDomElement root = doc.documentElement();

	// Read parts
	int partCount=0;
	QDomNodeList parts = root.childNodes();
	for(int i=0; i<parts.count(); i++) {
		QDomNode node = parts.at(i);
		if(node.nodeName()=="part") partCount += readPart(&node);
	}

	// Read animations
	for(int i=0; i<parts.count(); i++) {
		QDomNode node = parts.at(i);
		if(node.nodeName()=="animation") {
			QDomNamedNodeMap attr = node.attributes();
			QString name = attr.namedItem("name").nodeValue();
			int frames = attr.namedItem("length").nodeValue().toInt();
			float fps = attr.namedItem("fps").nodeValue().toFloat();
			bool loop = attr.namedItem("loop").nodeValue().toInt();
			Animation* anim = new Animation();
			anim->setName(name);
			anim->setFrameCount(frames<1?1:frames);
			anim->setFrameRate(fps>0?fps:15.0);
			anim->setLoop(loop);
			//Read parts
			QDomNodeList parts = node.childNodes();
			for(int p=0; p<parts.count(); p++) {
				int id = parts.at(p).attributes().namedItem("id").nodeValue().toInt();
				Part* part = getPart(id);
				//Read keyframes
				if(part) for(int j=0; j<parts.at(p).childNodes().count(); j++) {
					QDomNamedNodeMap data = parts.at(p).childNodes().at(j).attributes();
					Frame frame; frame.mode=0;
					frame.frame = data.namedItem("number").nodeValue().toInt();
					frame.mode |= data.namedItem("angle").isNull()?  0: 1;
					frame.mode |= data.namedItem("offset").isNull()? 0: 2;
					frame.mode |= data.namedItem("hidden").isNull()? 0: 4;
					//Values
					frame.angle = data.namedItem("angle").nodeValue().toFloat();
					frame.offset = readPoint( data.namedItem("offset") );
					frame.visible = !data.namedItem("hidden").nodeValue().toInt();
					//Add frame to animation
					anim->setKeyframe(frame.frame, part, frame);
				}
			}
			//Add animation to project
			addAnimation( anim );
		}
	}

	printf("Loaded %d parts and %d animations\n", m_parts.size(), m_animations.size());

	return true;
}

inline QPointF Project::readPoint(const QDomNode& node) const {
	return QPointF( node.nodeValue().section(',',0,0).toFloat(), node.nodeValue().section(',',1,1).toFloat() );
}

int Project::readPart(QDomNode* node, int parent) {
	QDomNamedNodeMap attr = node->attributes();
	int	id	= attr.namedItem("id").nodeValue().toInt();
	QString name	= attr.namedItem("name").nodeValue();
	QString file	= attr.namedItem("file").nodeValue();
	//pivot and offset need a bit more work
	QPointF pivot  = readPoint( attr.namedItem("pivot") );
	QPointF offset = readPoint( attr.namedItem("offset") );
	int     hidden = attr.namedItem("hidden").nodeValue().toInt();
	//Fix image path
	QDir base( m_file.section('/',0,-2) );
	file = base.absoluteFilePath(file);

	//Create part
	Part* part = createPart(name, id);
	part->setSource( file );
	//part->setImage( QPixmap(file) ); //TODO: xcf layer loading 'file.xcf:layer'
	part->setImage( loadGraphic(file) );
	addPart(part, getPart(parent));
	part->setOffset(-pivot.x(), -pivot.y());
	part->setRest(offset);
	part->setHidden(hidden);
	if(part->getParent()) part->setPos( part->getParent()->pos() + offset);
	else part->setPos(offset);

	//Sort out initial z order
	int z = attr.namedItem("z").nodeValue().toInt();
	part->setData(1,z); //store z
	QList<QGraphicsItem*> list = m_scene.items();
	for(int i=list.size()-1; i>0; i--) {
		if(list[i]->data(1).toInt() < z) {
			part->stackBefore(list[i]);
			break;
		}
	}

	//Recurse to children
	int count = 1;
	if(node->hasChildNodes()) {
		for(int i=0; i<node->childNodes().count(); i++) {
			QDomNode child = node->childNodes().at(i);
			count += readPart(&child, id);
		}
	}
	return count;
}

bool Project::saveProject(const QString& filename) {
	m_file = filename;
	//Create document
	QDomDocument doc;
	QDomProcessingInstruction decl = doc.createProcessingInstruction("xml", "version=\"1.0\"");
	doc.appendChild(decl);
	QDomElement root = doc.createElement("project");
	doc.appendChild(root);
	//Add parts
	for(QMap<int, Part*>::iterator i=m_parts.begin(); i!=m_parts.end(); i++) {
		if(!(*i)->getParent()) writePart(&root, *i);
	}
	//Add animations
	for(int i=0; i<m_animations.size(); i++) {
		Animation* anim = m_animations[i];
		QDomElement aNode = doc.createElement("animation");
		aNode.setAttribute("name", anim->name());
		aNode.setAttribute("length", anim->frameCount());
		aNode.setAttribute("fps", anim->frameRate());
		aNode.setAttribute("loop", anim->loop()?1:0);
		//Add part
		for(QMap<int, Part*>::iterator p=m_parts.begin(); p!=m_parts.end(); p++) {
			QDomElement pNode = doc.createElement("part");
			pNode.setAttribute("id", p.key());
			//Add frames
			int keyframes = 0;
			for(int j=0; j<anim->frameCount(); j++) {
				if(anim->isKeyframe(j, *p)) {
					Frame frame = anim->frameData(j, *p);
					if(frame.mode) {
						QDomElement fNode = doc.createElement("frame");
						fNode.setAttribute("number", frame.frame);
						if(frame.mode&1) fNode.setAttribute("angle", frame.angle);
						if(frame.mode&2) fNode.setAttribute("offset", QString("%1,%2").arg(frame.offset.x()).arg(frame.offset.y()));
						if(frame.mode&4) fNode.setAttribute("hidden", frame.visible?0:1);
						pNode.appendChild(fNode);
						keyframes++;
					}
				}
			}
			if(keyframes) aNode.appendChild(pNode);
		}
		root.appendChild(aNode);
	}

	// Write file
	QFile file(filename);
	if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) { m_file=QString::null; return false; }
	QTextStream stream( &file );
	stream << doc.toString();
	file.close();
}

int Project::writePart(QDomNode* parentNode, Part* part) {
	QDir base( m_file.section('/',0,-2) );
	QDomElement node = parentNode->ownerDocument().createElement("part");
	node.setAttribute("id", part->getID());
	node.setAttribute("name", part->getName());
	node.setAttribute("file", base.relativeFilePath( part->getSource() ));
	node.setAttribute("pivot", QString("%1,%2").arg( -part->offset().x() ).arg( -part->offset().y() ));
	node.setAttribute("offset", QString("%1,%2").arg( part->rest().x() ).arg( part->rest().y() ));
	node.setAttribute("z", m_scene.items().indexOf(part));
	if(part->hidden()) node.setAttribute("hidden", 1);
	//Children
	for(QList<Part*>::Iterator i=part->children().begin(); i!=part->children().end(); ++i) {
		writePart(&node, *i);
	}
	parentNode->appendChild(node);
}

//// //// //// //// //// //// //// //// XCF Images //// //// //// //// //// //// //// ////

QPixmap Project::loadGraphic(const QString& filename) {
	//is this an xcf file with a layer specified?
	if(filename.contains(".xcf:")) {
		QString layer = filename.section(':',-1,-1);
		QString file = filename.section(':',0,-2);
		//Find xcf image
		XCF* xcf;
		if(m_xcfImages.find(file)==m_xcfImages.end()) {
			xcf = new XCF();
			if(!xcf->load( file.toAscii().data() )) return QPixmap();
			m_xcfImages[file] = xcf;
		} else xcf = m_xcfImages[file];
		//Find the layer
		for(int i=0; i<xcf->layerCount; i++) if(layer == xcf->layer[i].name) {
			//Create QPixmap from layer data
			//printf("Found Layer %s", xcf->layer[i].name);
			return makeImage(xcf->layer[i].data, xcf->layer[i].width, xcf->layer[i].height, xcf->layer[i].bpp);
		}
		printf("Failed to find %s in %s\n", layer.toAscii().data(), file.toAscii().data());
		return QPixmap();
	} else return QPixmap(filename);
}

int Project::importXCF(const QString& file) {
	XCF* xcf = new XCF();
	if(!xcf->load( file.toAscii().data() )) return 0;
	// Load layers
	Part* behind = 0;
	for(int i=0; i<xcf->layerCount; i++) {
		printf("XCF Layer %d: %s\n", i, xcf->layer[i].name);

		QPixmap image = makeImage(xcf->layer[i].data, xcf->layer[i].width, xcf->layer[i].height, xcf->layer[i].bpp);

		//Create part
		Part* part = createPart( xcf->layer[i].name );
		part->setImage( image );
		part->setRest( QPointF( xcf->layer[i].x, xcf->layer[i].y) );
		part->setPos( part->rest() );
		part->setSource(file + ":" + part->getName());
		addPart(part);
		//Fix Initial order
		if(behind) part->stackBefore(behind);
		behind = part;
	}
	return xcf->layerCount;
}

QPixmap Project::makeImage(const unsigned char* data, int w, int h, int bpp) {
	//Hack data into acceptable form
	unsigned char* d = new unsigned char[w*h*4];
	for(int j=0; j<w*h; j++) {
		d[j*4+0] = data[j*4+2]; // B
		d[j*4+1] = data[j*4+1]; // G
		d[j*4+2] = data[j*4+0]; // R
		d[j*4+3] = bpp==4? data[j*4+3]: 0xff; // A
	}
	QImage image(d, w, h, QImage::Format_ARGB32);
	QPixmap pix = QPixmap::fromImage(image);
	delete [] d;
	return pix;

}
