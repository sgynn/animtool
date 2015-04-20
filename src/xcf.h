#ifndef _XCF_IMAGE_
#define _XCF_IMAGE_

#include <cstdio>

/** Basic xcf file loader / saver */
class XCF {
	public:
	XCF(): width(0), height(0), layerCount(0), layer(0) {};
	~XCF() { clear(); }

	bool load(const char* filename);
	bool save(const char* filename);
	void clear();

	//Image data - only support rgba
	unsigned int width, height;
	unsigned int bpp;
	unsigned int layerCount;
	struct Layer {
		const char* name;
		int bpp;  //bytes per pixel
		int x, y; //Offsets
		int width, height; //Size
		int blend; //Blend mode
		unsigned char* data;
	}* layer;

	private:

	class ByteStream {
		public:
		ByteStream(size_t length=32);
		ByteStream(const char* s, size_t length);
		~ByteStream();
		void append(const ByteStream& stream);
		size_t length() const { return m_size; }
		const char* data() const { return m_data; }
		size_t read (void* data, size_t size);
		size_t write(void* data, size_t size);
		template<typename T> size_t read (T* data) { return  read(data, sizeof(data)); }
		template<typename T> size_t write(T* data) { return write(data, sizeof(data)); }
		protected:
		size_t m_size;
		size_t m_buffer;
		size_t m_ptr;
		char* m_data;
	};

	bool readLayer(FILE* fp, Layer* layer);
	unsigned char* readLevel(FILE* fp, unsigned int width, unsigned int height, unsigned int bpp);
	unsigned int m_fileLength;

	unsigned char* writeLayer(FILE* fp, Layer* layer);
	unsigned char* writeLevel(FILE* fp, Layer* layer);

};

#endif

