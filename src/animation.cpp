#include "animation.h"
#include "part.h"

#include <cstdio>
#include <math.h>

Frame Animation::nullFrame;
Frame Animation::nullFrameHidden;

Animation::Animation() : m_loop(1), m_frameCount(1), m_rate(15.0) {
	memset(&nullFrame, 0, sizeof(Frame));
	memset(&nullFrameHidden, 0, sizeof(Frame));
	nullFrame.visible = true;
}
Animation::~Animation() {
}

QList<int> Animation::parts() const {
	QList<int> list;
	for(PartMap::const_iterator i=m_frames.begin(); i!=m_frames.end(); i++) {
		list.push_back(i.key());
	}
	return list;
}

Animation::PartAnim& Animation::partList(Part* part) {
	int id = part->getID();
	//Get part animation data
	return m_frames[id];
}

int Animation::setKeyframe(int frame, Part* part, Frame& data) {
	//Insert frame into list
	data.frame = frame;
	PartAnim& list = partList(part);
	for(PartAnim::Iterator i=list.begin(); i!=list.end(); i++) {
		if(i->frame == frame) { *i = data; return list.size(); }
		else if(i->frame > frame) { list.insert(i, data); return list.size(); }
	}
	list.push_back(data);
	return list.size();
}

int Animation::isKeyframe(int frame) {
	int key = 0;
	for(PartMap::const_iterator p=m_frames.begin(); p!=m_frames.end(); p++) {
		for(PartAnim::const_iterator i = p->begin(); i!=p->end(); i++) {
			if(i->frame==frame) key |= i->mode;
			if(i->frame>=frame) break;
		}
	}
	return key;
}
int Animation::isKeyframe(int frame, Part* part) {
	PartAnim& list = partList(part);
	for(PartAnim::Iterator i=list.begin(); i!=list.end(); i++) {
		if(i->frame==frame) return i->mode;
	}
	return 0;
}

Frame Animation::frameData(int frame, Part* part) {
	PartAnim& list = partList(part);
	if(list.empty()) return part->hidden()? nullFrameHidden: nullFrame;

	//Output frame
	Frame out;
	out.frame = frame;
	out.mode = 0;

	//Get keyframes for each part
	const Frame* a[3] = {0,0,0};
	const Frame* b[3] = {0,0,0};
	for(PartAnim::iterator i=list.begin(); i!=list.end(); i++) {
		for(int m=0; m<3; m++) {
			if(i->mode & (1<<m)) {
				if(m_loop && !b[m]) b[m] = &(*i); //first key for looping
				if(i->frame<=frame) a[m] = &(*i); //previous key
				if(i->frame>=frame && (!b[m]||b[m]->frame<frame)) b[m] = &(*i); //next key
				if(m_loop && (!a[m] || a[m]->frame>frame))        a[m] = &(*i); //Last key for looping
			}
		}
		//if there is a keyframe there, copy mode
		if(i->frame==frame) out.mode = i->mode;
	}
	//Deal with void fon non loops
	for(int i=0; i<3; i++) {
		if( !a[i] ) a[i] = b[i];
		if( !b[i] ) b[i] = a[i];
		if( !a[i] ) a[i] = b[i] = &nullFrame;
	}

	//Interpolate values
	out.angle =    interpolate(frame, a[0]->frame, b[0]->frame, a[0]->angle, b[0]->angle, true);
	out.offset.rx() = interpolate(frame, a[1]->frame, b[1]->frame, a[1]->offset.x(), b[1]->offset.x());
	out.offset.ry() = interpolate(frame, a[1]->frame, b[1]->frame, a[1]->offset.y(), b[1]->offset.y());
	out.visible = a[2]==&nullFrame? !part->hidden(): a[2]->frame<=b[2]->frame?a[2]->visible: b[2]->visible;

	return out;
}

float Animation::interpolate(int frame, int fa, int fb, float a, float b, bool angle) const {
	if(frame==fa || fa==fb) return a;
	if(fb<fa) fb += m_frameCount;
	if(frame<fa) frame += m_frameCount;
	//Wrap angle?
	if(angle && fabs(a-b)>180) a += (a<b? 360: -360);
	float t = (float)(frame-fa)/(fb-fa);
	return a + (b-a) * t;
}


void Animation::insertFrame(int index) {
	for(PartMap::Iterator p = m_frames.begin(); p!=m_frames.end(); p++) {
		for(PartAnim::Iterator i=p->begin(); i!=p->end(); i++) {
			if(i->frame >= index) i->frame++;
		}
	}
	setFrameCount( m_frameCount+1 );
}
void Animation::deleteFrame(int index) {
	for(PartMap::iterator p = m_frames.begin(); p!=m_frames.end(); p++) {
		for(PartAnim::Iterator i=p->begin(); i!=p->end(); i++) {
			if(i->frame==index) {
				i = p->erase(i);
				if(i==p->end()) break; //stop it going over the end
			}
			if(i->frame > index) i->frame--;
		}
	}
	setFrameCount( m_frameCount-1 );
}

//// //// //// //// //// //// //// //// Image Cache //// //// //// //// //// //// //// ////

bool Animation::hasCache(int frame) {
	return m_cache.size()>frame && !m_cache[frame].image.isNull();
}
void Animation::cacheFrame(int frame, QPixmap* image, const QPoint& point) {
	// Resize cache
	while(m_cache.size() > frameCount()) m_cache.pop_back();
	while(m_cache.size() < frame) m_cache.push_back( CachedImage() );
	// Cache frame
	CachedImage data;
	data.image = *image;
	data.point = point;
	m_cache.insert(frame, data);
}
const Animation::CachedImage& Animation::getCachedImage(int frame) {
	return m_cache.at(frame);
}


