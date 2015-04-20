#include "xcf.h"

#include <cstdio>
#include <cstring>
#include <vector>
#include <math.h>

//Reference: http://svn.gnome.org/viewvc/gimp/trunk/devel-docs/xcf.txt?view=markup

#define PROP_COLORMAP	 1
#define PROP_COMPRESSION 17
#define PROP_GUIDES	 18
#define PROP_RESOLUTION	 19
#define PROP_UNIT	 22
#define PROP_PATHS	 23
#define PROP_USER_UNIT	 24
#define PROP_VECTORS	 25

#define PROP_ACTIVE_LAYER	2
#define PROP_FLOATING_SELECTION 5
#define PROP_MODE		6
#define PROP_PRESERVE_TRANSPARENCY 10
#define PROP_APPLY_MASK		11
#define PROP_EDIT_MASK		12
#define PROP_SHOW_MASK		13
#define PROP_OFFSETS		15
#define PROP_TEXT_LAYER_FLAGS	26

#define PROP_END	 0
#define PROP_OPACITY	 6
#define PROP_VISIBLE	 8
#define PROP_LINKED	 9
#define PROP_TATTOO	 20
#define PROP_PARASITES	 21




//typedef unsigned int xcf_uint32;
typedef unsigned char xcf_byte;

#ifdef BIGENDIAN
typedef unsigned int xcf_uint32;
#else
struct xcf_uint32 {
	xcf_byte b[4];
	operator unsigned int() { return b[0]<<24 | b[1]<<16 | b[2]<<8 | b[3]; }
};
#endif


void XCF::clear() {
	//delete data
}

template<typename T> int read(T& p, FILE* fp) { return fread(&p, sizeof(T), 1, fp); }
template<typename T> int read(T* p, int count, FILE* fp) { return fread(p, sizeof(T), count, fp); }

bool XCF::load(const char* filename) {

	FILE* fp = fopen(filename, "rb");
	if(!fp) return false;

	//get file length
	fseek(fp, 0, SEEK_END);
	m_fileLength = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	//Version Header
	char magic[14];
	fread( magic, sizeof(magic), 1, fp ); 
	if(strncmp(magic, "gimp xcf ", 9)!=0) {
		printf("Invalid xcf file\n");
		fclose(fp);
		return false;
	}
	printf("xcf version: %s\n", magic+9);
	//Canvas properties
	xcf_uint32 canvas[3];
	fread(canvas, sizeof(canvas), 1, fp);
	width = canvas[0];
	height = canvas[1];
	//Check canvas Format
	if(canvas[2]!=0) {
		printf("xcf loader currently only reads RGB images\n");
		fclose(fp);
		return false;
	}

	//printf("width: %d  height: %d\n", width, height);

	bool useRLE = false; //Use RLE Compression

	//Properties...
	xcf_uint32  propStat[2];
	xcf_uint32* propData;
	while(!feof(fp)) {
		fread(propStat, sizeof(propStat), 1, fp);
		if(propStat[0] == PROP_END) break;
		if(propStat[1] > 0) {
			propData = new xcf_uint32[ propStat[1] ];
			fread(propData, propStat[1], 1, fp);
		}
		switch(propStat[0]) {
			case PROP_COMPRESSION:
				useRLE = propData[0];
				//printf("Image Property useRLE = %s\n", useRLE?"true":"false");
				break;
			default:
				//printf("Image Property #%d\n", (int)propStat[0]);
				break;
		}
		if(propStat[1] > 0) delete [] propData;
	}

	//Get layer pointers
	std::vector<unsigned int> layerPointers;
	std::vector<unsigned int> channelPointers;
	xcf_uint32 ptr;
	while(!feof(fp)) {
		fread(&ptr, sizeof(ptr), 1, fp);
		if(ptr>0) layerPointers.push_back(ptr);
		else break;
	}
	while(!feof(fp)) {
		fread(&ptr, sizeof(ptr), 1, fp);
		if(ptr>0) channelPointers.push_back(ptr);
		else break;
	}


	printf("%lu layers, %lu channels\n", layerPointers.size(), channelPointers.size());
	
	// Create layer output structure and load layers
	layerCount = layerPointers.size();
	layer = new Layer[ layerCount ];
	for(size_t i=0; i<layerPointers.size(); i++) {
		fseek(fp, layerPointers[i], SEEK_SET);
		readLayer(fp, layer+i);
	}


	fclose(fp);
	return true;
}

bool XCF::readLayer(FILE* fp, Layer* layer) {
	//Format: width, height, type, name, properties, pixel-heirachy-pointer, mask-pointer
	
	//const char* modeNames[6] = { "RGBA", "RGB", "Greyscale", "Grayscale Alpha", "Indexed", "Indexed Alpha" };
	
	xcf_uint32 type[3];
	fread(type, sizeof(type), 1, fp);
	layer->width = type[0];
	layer->height = type[1];
	//type[2] id the data type
	
	//Read name
	xcf_uint32 len;
	fread(&len, sizeof(len),1, fp);
	char* name = new char[len];
	fread(name, len, 1, fp);
	layer->name = name;

	//Properties
	xcf_uint32  propStat[2];
	xcf_uint32* propData;
	while(!feof(fp)) {
		fread(propStat, sizeof(propStat), 1, fp);
		if(propStat[0]==PROP_END) break;
		if(propStat[1]>0) {
			propData = new xcf_uint32[ ((int)propStat[1]) / 4 ];
			fread(propData, propStat[1], 1, fp);
			//printf("Layer Property #%d\n", (int)propStat[0]);

		}
		switch(propStat[0]) {
			case PROP_OFFSETS:
				layer->x = propData[0];
				layer->y = propData[1];
				break;
			case PROP_MODE:
				layer->blend = propData[0];
				break;
			default: break;
		}
		if(propStat[1]>0) delete [] propData;
	}

	//Data pointers
	xcf_uint32 pointers[2];
	fread(pointers, sizeof(pointers), 1, fp);


	//printf("Layer %s: %dx%d  offset %d,%d type:%s\n", layer->name, layer->width, layer->height, layer->x, layer->y, modeNames[ type[2] ]);

	//Read pixel data ....
	//Looks like it gets a bit more complicated
	
	//int modes[6] = { 3, 4, 1, 2, 1, 2 };

	//xcf format has multiple mip levels, but only the first on is used.
	fseek(fp, pointers[0], SEEK_SET);
	xcf_uint32 levelStat[5]; //width, height, bpp, ptr, ptr
	fread(levelStat, sizeof(levelStat), 1, fp);
	fseek(fp, levelStat[3], SEEK_SET);

	//printf("Level: %#x - %#x\n", (int)levelStat[3], (int)levelStat[4]);

	layer->data = readLevel(fp, levelStat[0], levelStat[1], levelStat[2]);

	//Save bpp
	layer->bpp = levelStat[2];

	return layer->data!=0;
}

xcf_byte* XCF::readLevel(FILE* fp, unsigned int width, unsigned int height, unsigned int bpp) {
	//Size again
	xcf_uint32 size[2];
	fread(size, sizeof(size), 1, fp);
	if(size[0]!=width || size[1]!=height) {
		printf("Size mismatch [%u,%u] [%u,%u]\n", width, height, (int)size[0], (int)size[1]);
		return 0;
	}
	//Tile pointers
	int tileCount = ceil(width/64.0) * ceil(height/64.0) + 1;
	xcf_uint32* tilePtr = new xcf_uint32[tileCount];
	fread(tilePtr, tileCount*sizeof(xcf_uint32), 1, fp);

	//printf("%d Tiles %dbpp\n", tileCount-1,bpp);

	xcf_byte* data = new xcf_byte[ width * height * bpp ];
	memset(data, 0, width*height*bpp); //Dont need this
	int across = ceil(width/64.0);
	
	xcf_byte* buffer = new xcf_byte[ 64*64*bpp*2]; //is there a better way for this?


	//Read the tiles
	for(int i=0; i<tileCount-1; i++) {
		fseek(fp, tilePtr[i], SEEK_SET);
		int len = tilePtr[i+1]-tilePtr[i];
		if(len<0) len=64*64*bpp*2; //need to get a value for this

		unsigned int tx = (i%across)*64;
		unsigned int ty = (i/across)*64;
		unsigned int tw = width-tx>64? 64: width-tx;
		unsigned int th = height-ty>64? 64: height-ty;

		len = fread(buffer, 1, len, fp);
		//printf("Read %d [%#x]\n", len, (int)tilePtr[i]);

		xcf_byte* tileStart = data + tx*bpp + ty*width*bpp;
		xcf_byte* tileEnd   = data + (tx+tw)*bpp + (ty+th-1)*width*bpp;
		xcf_byte* rowEnd = tileStart + tw*bpp;
		#define out(c)	{ *d=c; d+=bpp; if(d==rowEnd) { d += (width-tw)*bpp; rowEnd+=width*bpp; } }

		//printf("Tile [%d,%d] %dx%d %lu - %lu\n", tx,ty, tw,th, tileStart-data, tileEnd-data);

		unsigned int channel = 0;
		xcf_byte* c = buffer;
		xcf_byte* d = tileStart;
		while(c<buffer+len) {
			if(*c<=126) { //short run
			//	printf("Run: %x (%d)\n", c[1], c[0]);
				for(int j=0; j<=*c; j++) out( c[1] );
				c+=2;
			} else if(*c==127) { //long run
				int r = c[1]*256+c[2];
			//	printf("Run: %x (%d)\n", c[3], r);
				for(int j=0; j<r; j++) out( c[3] );
				c+= 4;
			} else if(*c==128) { //long chunk
				int r = c[1]*256+c[2];
				c+=3;
			//	printf("Chunk %d\n", r);
				for(int j=0; j<r; j++,c++) out(*c);
			} else { //short chunk
				int r = 256 - *c; c++;
			//	printf("Chunk %d\n", r);
				for(int j=0; j<r; j++,c++) out(*c);
			}
			//End of channel?
			if(d>=tileEnd) {
			//	printf("---\n");
				if(++channel==bpp) break; //End
				d = tileStart + channel;
				rowEnd = d + tw * bpp;
			}
		}
	}
	return data;
}

//// //// //// //// //// //// //// //// Writing methods //// //// //// //// //// //// //// ////

bool XCF::save(const char* filename) {
	return false;
}

xcf_byte* XCF::writeLayer(FILE* fp, Layer* layer) {
	return 0;
}
xcf_byte* XCF::writeLevel(FILE* fp, Layer* layer) {
	return 0;
}









