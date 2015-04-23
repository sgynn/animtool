#ifndef _ANIMATION_
#define _ANIMATION_

#include <QPoint>
#include <QString>
#include <QImage>
#include <QPixmap>
#include <QMap>

class Part;

struct Frame {
	int frame;	//Frame number
	int mode;	//Which data is keyed
	QPointF offset;	//Part offset
	float angle;	//Part angle;
	bool visible;	//Part visible?
};

class Animation {
	public:
	Animation();
	~Animation();

	const QString& name() const { return m_name; }		// Get animation name
	void setName(const QString& s) { m_name=s; }		// Set animation name

	float frameRate() const { return m_rate; }		// Get frame rate
	void setFrameRate(float fps) { m_rate = fps; }		// Set frame rate

	int frameCount() const { return m_frameCount; }		// Get the number of frames
	void setFrameCount(int c) { m_frameCount = c; }		// Set the number of frames

	void setLoop(bool loop) { m_loop=loop; }		// Set whether tha animation loops
	bool loop() const { return m_loop; }			// Does the animation loop

	enum FrameType { NONE=0, ANGLE=1, POS=2, VIS=4 };	// Keyframe elements
	int setKeyframe(int frame, Part* part, Frame& data);	// Set a keyframe
	int isKeyframe(int frame, Part* part);			// Is a frame a keyframe
	int isKeyframe(int frame);				// Is a frame keyed on any parts
	Frame frameData(int frame, Part* part);			// Get interpolated data for a frame

	QList<int> parts() const;				// Get a list of all the parts with keyframes

	void setControllerState(int id, bool active);	// Set theactive state of a controller
	bool getControllerState(int id) const;			// Is a controller active?

	void insertFrame(int index);				// Insert a frame at index
	void deleteFrame(int index);				// delete a frame
	
	int getID() const { return m_id; }			// Get animation ID
	void setID(int id) { m_id = id; }			// Set animation ID

	struct CachedImage { QPixmap image; QPoint point; };
	void cacheFrame(int frame, QPixmap*, const QPoint&);	// Cache a pre-rendered animation frame
	bool hasCache(int frame);				// Is this frame cached?
	const CachedImage& getCachedImage(int frame);		// Get frame image

	static Frame nullFrame;			//Null frame
	static Frame nullFrameHidden;		//Null frame
	private:
	int m_id;				// Animation ID
	QString m_name;				// Animation name
	bool m_loop;				// Is this animation looped?
	int m_frameCount;			// Number of frames
	float m_rate;				// Playback rate (fps)
	typedef QList<Frame> PartAnim;		// Animation data struct
	typedef QMap<int, PartAnim> PartMap;	// Part animation map
	PartMap m_frames;			// One animation per part

	QMap<int, bool> m_ikStates;	// Which ik controllers are active for this animation

	QList<CachedImage> m_cache;		// Rendered frame images

	PartAnim& partList(Part* part);		// Get/create animation frame list for a part
	float interpolate(float frame, int fa, int fb, float va, float vb, bool angle=false) const;
};

#endif

