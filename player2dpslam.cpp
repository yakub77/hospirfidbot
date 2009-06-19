#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 128

enum State {WAITING_ODOMETRY, WAITING_LASER};

//Use this to make sure there's exactly one laser and odometry reading in each
//pair (dp slam requires that, but the number of laser readings DOES NOT equal the
//number of odometry readings in player); this helps to skip the extra readings
enum State state = WAITING_ODOMETRY;

void skipLine(FILE* fp) {
	char c;
	do {
		c = fgetc(fp); 
	} 
	while ((c >= 0) && (c != '\n'));
}

struct PlayerHeader {
	bool valid; //Is this a valid header? (it could be a comment)
	double time;
	unsigned int host;
	unsigned int robot;
	char interfaceName[BUFSIZE];
	unsigned int index;
	unsigned int type;
	unsigned int subtype;

	void readHeader(FILE* fp);
	void Print();
};

void PlayerHeader::readHeader(FILE* fp) {
	char buf[BUFSIZE];
	fscanf(fp, "%s", buf);
	if (buf[0] == '#') {
		// A comment is encountered, assume this is not a valid header
		// Skip everything in a line that's started with a comment
		skipLine(fp);
		valid = false;
		return;
	}
	valid = true;
	time = atof(buf);
	fscanf(fp, "%u%u%s%u%u%u", &host, &robot, interfaceName, &index, &type, &subtype);
}

void PlayerHeader::Print() {
	//printf("%lf %u %u %s %u %u %u\n", time, host, robot, interfaceName, index, type, subtype);
	if (!valid)
		printf("Line not valid\n");
	else	
		printf("%lf:%s\n", time, interfaceName);
}


/*  From DP_SLAM README, the output format needs to be:
LASER <number> <values>...
<number> is the number of laser readings that were observed. This 
should usually be 181. Those actual laser measurements are the values
that follow, in meters. The laser readings are assumed to start at 
-90 degrees, and occur at every 1 degree up to +90 degrees, with regard
to the robot's facing angle.*/
void outputLaser(FILE* fin, FILE* fout) {
	//Expected values:
	//-1.5708 +1.5708 +0.01745329 +5.6000 0181
	double startangle, endangle, increment, minrange, maxrange;
	unsigned int bins;
	char buf[BUFSIZE];
	fscanf(fin, "%lf%lf%lf%lf%lf%u", &startangle, &endangle, &increment, &minrange, &maxrange, &bins);
	//printf("%lf %lf %lf %lf %lf %u\n", startangle, endangle, increment, minrange, maxrange, bins);
	if (bins != 181) {
		fprintf(stderr, "Warning: Expected 181 bins, found %i; skipping line\n\n", bins);
		skipLine(fin);
		state = WAITING_LASER;
		return;	
	}
	fprintf(fout, "Laser 181 ");
	int counter = 0;
	while (counter < 181) {
		fscanf(fin, "%s", buf);
		fprintf(fout, "%s ", buf);
		counter++;
	}
	fprintf(fout, "\n");
	fflush(fout);
}

/* From DP_SLAM readme, odometry format needs to be:
ODOMETRY <x> <y> <theta>
The first argument denotes this as a reading from the robot's odometer. 
<x> and <y> are the robot's current position from some arbitrary 
starting point. These measures are in meters. <theta> is robot's 
current facing angle, in radians.*/
void outputOdometry(FILE* fin, FILE* fout) {
//1245349407.171 16777343 6665 position2d 00 001 001 +00.439 -00.027 -0.101 +00.000 +00.000 +00.000 0
	double x, y, theta;
	fprintf(fout, "Odometry \n");
	fscanf(fin, "%lf%lf%lf", &x, &y, &theta);
	fprintf(fout, "%lf %lf %lf\n", x, y, theta);
	skipLine(fin);
	fflush(fout);
}

int main(int argc, char** argv) {
	char* filein, fileout;
	char buf[BUFSIZE];
	FILE* fin;
	FILE* fout;
	int host, robot;
	char interface[128];
	int index, type, subtype;
	struct PlayerHeader header;

	if (argc != 3) {
		fprintf(stderr, "Syntax: player2dpslam <filein> <fileout>");
		return 1;
	}
	fin = fopen(argv[1], "r");
	if (fin == NULL) {
		fprintf(stderr, "ERROR opening input file %s\n", argv[1]);
		return 1;
	}
	fout = fopen(argv[2], "w");
	if (fout == NULL) {
		fprintf(stderr, "ERROR opening output file %s\n", argv[2]);
		return 1;
	}
	while (!feof(fin)) {
		header.readHeader(fin);
		//header.Print();
		if (header.valid) {
			if (strcmp(header.interfaceName, "laser") == 0) {
				if (state == WAITING_LASER) {
					outputLaser(fin, fout);
					state = WAITING_ODOMETRY;				
				}
				else
					skipLine(fin);
			}
			else if (strcmp(header.interfaceName, "position2d") == 0) {
				if (state == WAITING_ODOMETRY) {				
					outputOdometry(fin, fout);
					state = WAITING_LASER;
				}
				else
					skipLine(fin);
			}
		}
		fflush(fout);
	}
	fflush(fout);
	return 0;
}
