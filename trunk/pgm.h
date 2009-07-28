/*Author: Chris Tralie
 *Project: Duke REU Fellowship 2009: Robotic navigation with RFID Waypoints
 *Purpose: To create a class that can read in binary data from a simple PGM
 *grayscale image and store the intensity values (0-255) in an array of bytes
 *where it can be easily manipulated (also provide some image processing functions)*/

#ifndef PGM_H
#define PGM_H

#include <iostream>
#include <fstream>
#include <string>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cstring>

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
	//The most important function; used to 
	void autoQuantize();
};


#endif
