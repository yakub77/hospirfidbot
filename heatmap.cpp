/*Author: Chris Tralie
 *Project: Duke REU Fellowship 2009: Robotic navigation with RFID Waypoints
 *Purpose: The program is given a PGM image map file, along with that map's resolution (in meters),
 *a logfile of odometry readings localized to that map (with system timestamps), and
 *a logfile of RFID tags seen (also with sytem epoch timestaps).  The program will
 *create a heatmap of every RFID tag seen that aligns with the map by matching up the 
 *two logfiles; that is, at every localized position, it will find all of the RFID tags that 
 *were seen at the time that position was recorded (or the closest time to that time that exists
 *in the RFID logfile)*/

#include "heatmap.h"

void skipLine(FILE* fp) {
	char c;
	do {
		c = fgetc(fp); 
	} 
	while ((c >= 0) && (c != '\n'));
}

//Given a localized coordinate, translate it into
//map coordinates with the getX() and getY() functions
int HeatMaps::getMapX(double x) {
	int temp = (int)(x / mapRes);
	return temp + originX;
}

int HeatMaps::getMapY(double y) {
	int temp = (int)(y / mapRes);
	temp = temp + originY;
	return height - temp;
}

//A helper function for the constructor; this function goes through
//and finds what RFID tags were seen when the robot thought it was 
//at each position, and then adds the readings to the heatmap
void HeatMaps::fillHeatMaps(const char* logfileamcl) {
	FILE* fp = fopen(logfileamcl, "r");
	if (fp == NULL) {
		fprintf(stderr, "ERROR: Unable to open localized logfile %s\n", logfileamcl);
		return;
	}
	char timebuf[128];
	char interface[128];
	double time;
	unsigned int host, robot, index, type, subtype;
	double x, y, theta;
	while (!feof(fp)) {
		//First read in the line of the logfile
		//1245954086.098 16777343 6665 position2d 01 001 001 +03.127 -01.183 -0.493 +00.000 +00.000 +00.000 0
		fscanf(fp, "%128s", timebuf);
		if (timebuf[0] == '#' || timebuf[0] == '\n') {//Ignore lines with comments
			skipLine(fp);
			continue;
		}
		time = atof(timebuf);
		//printf("%lf\n", time);
		fscanf(fp, "%u%u%128s%u%u%u%lf%lf%lf", &host, &robot, interface, 
			&index, &type, &subtype, &x, &y, &theta);
		skipLine(fp);
		int mapX = getMapX(x);
		int mapY = getMapY(y);
		//printf("(%f, %f)   (%i, %i) \n", x, y, mapX, mapY);
		//Get all of the tags that were read at the closest time to this position
		vector<tread> tagreadings = rfidlog->getClosestReadings(time);
		//Update the heatmaps of all of the tags seen to have a cell filled at this
		//position
		for (int i = 0; i < tagreadings.size(); i++) {
			int id = tagreadings[i].id;
			int strength = tagreadings[i].strength;
			if (heatmaps.find(id) == heatmaps.end()) {
				//A heatmap has not been made for that tag yet;
				//allocate space for one
				heatmaps[id] = new PGMImage(width, height);
				strongestPos[id].strength = strength;
				strongestPos[id].x = x;
				strongestPos[id].y = y;
			}
			else if (strength > strongestPos[id].strength) {
				strongestPos[id].strength = strength;
				strongestPos[id].x = x;
				strongestPos[id].y = y;
			}
			heatmaps[id]->setPixel(mapX, mapY, strength);
		}
	}
	printf("finished making heatmaps\n");
}

//Fill in all of the information needed to create heatmaps of every RFID tag found in
//in the log file "logfilerfid," that can be aligned with the PGM map contained in "mapfile"
//which has resolution "mres" per pixel (meters per pixel).  Use the localized position readings
//in the logfile "logfileamcl" to create the heatmaps
HeatMaps::HeatMaps(double mres, const char* mapfile, const char* logfileamcl, const char* logfilerfid) {
	mapRes = mres;
	mapimage = new PGMImage(mapfile);
	rfidlog = new RFIDLog(logfilerfid);
	width = mapimage->width;
	height = mapimage->height;
	//Assume (for now) that the origin is in the center of the image
	originX = width / 2;
	originY = height / 2;
	fillHeatMaps(logfileamcl);
}

HeatMaps::~HeatMaps() {
	delete mapimage;
	delete rfidlog;
	//Clean up heatmaps
	//printf("%i\n", heatmaps.size());
	map<int, PGMImage*>::iterator iter = heatmaps.begin();
	while (iter != heatmaps.end()) {
		//printf("%i ", iter->first);
		delete iter->second;
		iter++;
	}
}

//Write out a grayscale PGM heatmap of the tag with the assigned id "tagid"
//to the file "filename"
void HeatMaps::saveHeatMap(int tagid, const char* filename) {
	if (heatmaps.find(tagid) == heatmaps.end()) {
		fprintf(stderr, "ERROR: Did not see tag with id %i\n", tagid);
		return;
	}
	heatmaps[tagid]->writeToFile(filename);
}

//Used to save the position of the strongest tag reading for each tag (more precise than
//determining this later from the array of pixels)
//Format: 
//mapRes
//<tag assigned int id> <tag hex id> <max strength at centroid> <centroid x position (meters)> <centroid y position (meters)>
void HeatMaps::saveCentroids(const char* filename) {
	FILE* fp = fopen(filename, "w");
	if (fp == NULL) {
		fprintf(stderr, "ERROR: Unable to open file %s\n", filename);
		return;
	}
	fprintf(fp, "%lf\n", mapRes);
	map<int, struct Coord>::iterator iter = strongestPos.begin();
	while (iter != strongestPos.end()) {
		fprintf(fp, "%i %s %i %lf %lf\n", iter->first, rfidlog->reverseTags[iter->first].data(), iter->second.strength,
			iter->second.x, iter->second.y);
		iter++;
	}
}
