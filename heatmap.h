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

//Define a struct that can hold multiple readings of the same
//tag in one cell
struct TagReadingX {
	int id;
	int strength;
	int totalread;
};

typedef struct TagReadingX treadx;

class HeatMaps {
public:
	PGMImage mapimage;
	RFIDLog rfidlog;
	double mapRes;//The number of meters per cell
	int width, height;
	int originX, originY;
	
	//A two dimensional array of maps, whose resolution
	//matches the resolution of "map."  Each cell stores the
	//RFID tag readings found at that position
	vector<map<int, treadx> > heatmaps;
	
	HeatMaps(double mres, const char* mapfile, const char* logfileamcl, const char* logfilerfid);
	~HeatMaps();
	//Convert to map coordinates
	int getMapX(double x);
	int getMapY(double x);
	void fillHeatMaps(const char* logfileamcl);
	void saveHeatMap(int tagid, const char* filename);
};

#endif
