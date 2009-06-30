#ifndef PGM_H
#define PGM_H

#include <iostream>
#include <fstream>
#include <string>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

//typedef ifstream::pos_type ptype;
typedef int ptype;
typedef unsigned char BYTE;

class PGMImage {
public:
	int width, height, maxvalue;
	BYTE* imgdata;
	PGMImage(char* filename);
	PGMImage(int w, int h);
	~PGMImage();
	void parseData(BYTE* data, ptype size);
	void writeToFile(char* filename);
	
	//i is position along the width, j is position along the height
	//(0, 0) is upper left of image
	BYTE getPixel(int i, int j);
	void setPixel(int i, int j, BYTE value);
};


#endif
