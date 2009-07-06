#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "pgm.h"

using namespace std;

void skipLine(FILE* fp) {
	char c;
	do {
		c = fgetc(fp); 
	} 
	while ((c >= 0) && (c != '\n'));
}

struct FakeTag {
	int id;
	double x, y;
	double strength;
};

int main(int argc, char** argv) {
	if (argc != 5) {
		fprintf(stderr, "Usage: SimulateRFID <logfilein> <tagimage> <mapres> <logfileout>\n");
		return 1;
	}
	double mapres = atof(argv[3]);
	FILE* fin = fopen(argv[1], "r");
	FILE* fout = fopen(argv[4], "w");
	if (fin == NULL) {
		fprintf(stderr, "ERROR opening input file %s\n", argv[1]);
		return 1;
	}
	if (fout == NULL) {
		fprintf(stderr, "ERROR opening output file %s\n", argv[4]);
		return 1;
	}
	PGMImage tagimage(argv[2]);
	vector<struct FakeTag> tags;
	//Initialize tags
	for (int i = 0; i < tagimage.width; i++) {
		for (int j = 0; j < tagimage.height; j++) {
			if (tagimage.getPixel(i, j) < 255) {
				struct FakeTag coord;
				double x = (i - tagimage.width / 2) * mapres;
				double y = (j - tagimage.height / 2) * mapres;
				coord.x = x;
				coord.y = y;
				tags.push_back(coord);
			}
		}
	}
	
	//Now go through and run the "simulation"
	while (!feof(fin)) {
		char timebuf[128];
		char interface[128];
		double time;
		unsigned int host, robot, index, type, subtype;
		double x, y, theta;
		fscanf(fin, "%128s", timebuf);
		if (timebuf[0] == '#') {//Ignore lines with comments
			skipLine(fin);
			continue;
		}
		time = atof(timebuf);
		fscanf(fin, "%u%u%128s%u%u%u%lf%lf%lf", &host, &robot, interface, 
			&index, &type, &subtype, &x, &y, &theta);
		skipLine(fin);
		vector<struct FakeTag> visibletags;
		double min_decay = 0.1;
		for (int i = 0; i < tags.size(); i++) {
			//TODO:
			double dx = tags[i].x - x;
			double dy = tags[i].y - y;
			double rSquared = dx*dx + dy*dy;
			double decay = 1.0 / (1 + (0.5) * rSquared);
			if (decay > min_decay) {
				struct FakeTag visibletag;
				visibletag.id = i;
				visibletag.strength = decay;
				visibletags.push_back(visibletag);
			}
		}
		//Now write out the visible tags to a line in the logfile, using the following syntax:
		//<time> <#tags> <string hex id> <strength 1> ..... 
		fprintf(fout, "%lf %i ", time, visibletags.size());
		for (int i = 0; i < visibletags.size(); i++) {
			fprintf(fout, "%i %i ", visibletags[i].id, (int)(visibletags[i].strength * 255));
		}
		fprintf(fout, "\n");
	}
	return 0;
}
