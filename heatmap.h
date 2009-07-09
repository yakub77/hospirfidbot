#include "pgm.h"
#include "rfid.h"
#include <vector>
#include <map>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#ifndef HEATMAP_H
#define HEATMAP_H

using namespace std;

//Use this to store the coordinate of the strongest RFID reading
struct Coord {
	int strength;
	double x, y;
};

class HeatMaps {
public:
	PGMImage* mapimage;
	RFIDLog* rfidlog;
	double mapRes;//The number of meters per cell
	int width, height;
	int originX, originY;
	
	//This map stores an image for every tag ID
	map<int, PGMImage*> heatmaps;
	map<int, struct Coord> strongestPos;
	
	HeatMaps(double mres, const char* mapfile, const char* logfileamcl, const char* logfilerfid);
	~HeatMaps();
	//Convert to map coordinates
	int getMapX(double x);
	int getMapY(double x);
	void fillHeatMaps(const char* logfileamcl);
	void saveHeatMap(int tagid, const char* filename);
	void saveCentroids(const char* filename);//Write out the "strongestPos" map to a file
};

#endif
