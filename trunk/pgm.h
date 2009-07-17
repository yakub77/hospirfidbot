#ifndef PGM_H
#define PGM_H

#include <iostream>
#include <fstream>
#include <string>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

using namespace std;

//typedef ifstream::pos_type ptype;
typedef int ptype;
typedef unsigned char BYTE;

class PGMImage {
public:
	int width, height, maxvalue;
	BYTE* imgdata;
	PGMImage(const char* filename);
	PGMImage(int w, int h);
	PGMImage();
	~PGMImage();
	void parseData(BYTE* data, ptype size);
	void writeToFile(const char* filename);
	
	//i is position along the width, j is position along the height
	//(0, 0) is upper left of image
	BYTE getPixel(int i, int j);
	void setPixel(int i, int j, BYTE value);
	void Quantize(BYTE cutoff);
	void Blur(int w);
	PGMImage getEdges();
	void autoQuantize();
};


#endif