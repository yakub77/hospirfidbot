/*Author: Chris Tralie
 *Project: Duke REU Fellowship 2009: Robotic navigation with RFID Waypoints
 *Purpose: To convert a logfile from player into a format accepted by the 
 *DP_SLAM, an occupancy grid builder created by Ronald Parr and Austin Eliazar*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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
	fscanf(fp, "%128s", buf);
	if (buf[0] == '#') {
		// A comment is encountered, assume this is not a valid header
		// Skip everything in a line that's started with a comment
		skipLine(fp);
		valid = false;
		return;
	}
	valid = true;
	time = atof(buf);
	fscanf(fp, "%u%u%128s%u%u%u", &host, &robot, interfaceName, &index, &type, &subtype);
}

void PlayerHeader::Print() {
	//printf("%lf %u %u %s %u %u %u\n", time, host, robot, interfaceName, index, type, subtype);
	if (!valid)
		printf("Line not valid\n");
	else	
		printf("%lf:%s\n", time, interfaceName);
}



void outputLaser(PlayerHeader h, FILE* fin, FILE* fout) {
	//Expected values:
	//-1.5708 +1.5708 +0.01745329 +5.6000 0181
	double startangle, endangle, increment, minrange, maxrange;
	unsigned int bins;
	fscanf(fin, "%lf%lf%lf%lf%lf%u", &minrange, &startangle, &endangle, &increment, &maxrange, &bins);
	//printf("startangle=%lf endangle=%lf increment=%lf minrange=%lf maxrange=%lf bins=%u\n", startangle, endangle, increment, minrange, maxrange, bins);
	if (bins != 181) {
		fprintf(stderr, "Warning: Expected 181 bins, found %i; skipping line\n\n", bins);
		skipLine(fin);
		state = WAITING_LASER;
		return;	
	}
	//1053653857.29 fly 6665 laser 00 1053652583.773 -1.571 +1.571 0.017453 181
	fprintf(fout, "%lf %u %u %s %u %u %u ", h.time, h.host, h.robot, h.interfaceName, h.index, h.type, h.subtype);
	fprintf(fout, "%lf %lf %lf %lf %lf %u ", minrange, startangle, endangle, increment, maxrange, bins);
	int counter = 0;
	char scanrange[128];
	char intensity[128];
	while (counter < 181) {
		fscanf(fin, "%128s", scanrange);
		fprintf(fout, "%s ", scanrange);
		fscanf(fin, "%128s", intensity);
		fprintf(fout, "%s ", intensity);
		counter++;
	}
	fprintf(fout, "\n");
	fflush(fout);
}

void outputOdometry(PlayerHeader h, FILE* fin, FILE* fout) {
//1245349407.171 fly 6665 position2d 00 001 001 +00.439 -00.027 -0.101 +00.000 +00.000 +00.000 0
//1053653857.290 fly 6665 position 00 1053652583.883 +01.200 -00.910 -1.501 -0.005 +0.000 +0.000
	float x, y, theta;
	float a, b, c, d;
	fprintf(fout, "%lf %u %u %s %u %u %u ", h.time, h.host, h.robot, h.interfaceName, h.index, h.type, h.subtype);
	fscanf(fin, "%f%f%f", &x, &y, &theta, &a, &b, &c, &d);
	float newtheta = sqrt(abs(theta));
	if (theta < 0) newtheta *= -1;
	fprintf(fout, "%.2f %.2f %.2f %.2f %.2f %.2f %.2f\n", x, y, newtheta, a, b, c, d);
	printf("%.2f\n", theta);
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
		fprintf(stderr, "Syntax: changeodometry <filein> <fileout>");
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
					outputLaser(header, fin, fout);
					//skipLine(fin);
					state = WAITING_ODOMETRY;				
				}
				else
					skipLine(fin);
			}
			else if (strcmp(header.interfaceName, "position2d") == 0) {
				if (state == WAITING_ODOMETRY) {				
					outputOdometry(header, fin, fout);
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
