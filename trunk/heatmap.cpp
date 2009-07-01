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
	return temp + originY;
}

//A helper function for the constructor; this function goes through
//and finds what RFID tags were seen when the robot thought it was 
//at each position, and then adds the readings to the heatmap
void HeatMaps::fillHeatMaps(const char* logfileamcl) {
	FILE* fp = fopen(logfileamcl, "r");
	char timebuf[128];
	char interface[128];
	double time;
	unsigned int host, robot, index, type, subtype;
	double x, y, theta;
	while (!feof(fp)) {
		//First read in the line of the logfile
		/*
		##   time     host   robot  interface index  type   subtype
		##   (double) (uint) (uint) (string)  (uint) (uint) (uint)
		## - Following the common header is the message payload 
		1245954086.098 16777343 6665 position2d 01 001 001 +03.127 -01.183 -0.493 +00.000 +00.000 +00.000 0*/
		fscanf(fp, "%128s", timebuf);
		if (timebuf[0] == '#') {//Ignore lines with comments
			skipLine(fp);
			continue;
		}
		time = atof(timebuf);
		fscanf(fp, "%u%u%s%u%u%u%lf%lf%lf", &host, &robot, interface, 
			&index, &type, &subtype, &x, &y, &theta);
		skipLine(fp);
		int mapX = getMapX(x);
		int mapY = getMapY(y);
		vector<tread> tagreadings = rfidlog.getClosestReadings(time);
		//j * width + i
		int index = mapY * width + mapX;
		for (int i = 0; i < tagreadings.size(); i++) {
			tread reading = tagreadings[i];
			//Look up the tag ID in that block to see if it's been read
			//in that block yet
			if (heatmaps[index].find(reading.id) != heatmaps[index].end()) {
				//There is already an entry for this tag in this block
				//Add this new reading so an average can be taken
				(heatmaps[index])[reading.id].strength += reading.strength;
				(heatmaps[index])[reading.id].totalread++;
			}
			else {
				//There is not any entry for this tag in this cell yet, 
				//so make a new one
				treadx newEntry;
				newEntry.id = reading.id;
				newEntry.strength = reading.strength;
				newEntry.totalread = 1;
				(heatmaps[index])[reading.id] = newEntry;
			}
		}
	}
}

//Fill in all of the information needed to create heatmaps of every RFID tag found in
//in the log file "logfilerfid," that can be aligned with the PGM map contained in "mapfile"
//which has resolution "mres" per pixel (meters per pixel).  Use the localized position readings
//in the logfile "logfileamcl" to create the heatmaps
HeatMaps::HeatMaps(double mres, const char* mapfile, const char* logfileamcl, const char* logfilerfid) {
	mapRes = mres;
	mapimage = PGMImage(mapfile);
	rfidlog = RFIDLog(logfilerfid);
	width = mapimage.width;
	height = mapimage.height;
	//Assume (for now) that the origin is in the center of the image
	originX = width / 2;
	originY = height / 2;
	
	//Initialize the dynamic structure that stores all of the heatmap info
	for (int i = 0; i < width * height; i++) {
		map<int, treadx> temp;
		heatmaps.push_back(temp);
	}
	fillHeatMaps(logfileamcl);
}

HeatMaps::~HeatMaps() {}

//Write out a grayscale PGM heatmap of the tag with the assigned id "tagid"
//to the file "filename"
void HeatMaps::saveHeatMap(int tagid, const char* filename) {
	PGMImage image(width, height);
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			int index = j*width + i;
			int strength = 0;
			//Look for the tag in this block
			if (heatmaps[index].find(tagid) != heatmaps[index].end()) {
				//The tag was seen in this block
				strength = (heatmaps[index])[tagid].strength;
				strength /= (heatmaps[index])[tagid].totalread;//Average
			}
			image.setPixel(i, j, strength);
		}
	}
	image.writeToFile(filename);
}

