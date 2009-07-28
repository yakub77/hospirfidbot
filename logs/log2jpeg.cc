/*Author: Chris Tralie
 *Project: Duke REU Fellowship 2009: Robotic navigation with RFID Waypoints
 *Purpose: When Player logs camera data to a logfile, it puts a big long hex 
 *string of all of the JPEG data next to each timestamp.  This program goes 
 *through the logfile and converts each hex string into an array of bytes, and 
 *then writes it out to a file for each JPEG image.  In other words, it converts 
 *the logfile from the camera into an array of JPEG images in some folder, which 
 *can then be accessed by ViewVideo.java to play them back*/

#include <stdio.h>
#include <stdlib.h>

#define BUFSIZE 128

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
	
	//640 480 24 5 1 42149
	int resx, resy;
	int depth;
	int u1, u2;
	unsigned int length;

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
	valid = fscanf(fp, "%u%u%128s%u%u%u", &host, &robot, interfaceName, &index, &type, &subtype);
	valid = fscanf(fp, "%i%i%i%i%i%u", &resx, &resy, &depth, &u1, &u2, &length);
}

void PlayerHeader::Print() {
	//printf("%lf %u %u %s %u %u %u\n", time, host, robot, interfaceName, index, type, subtype);
	if (!valid)
		printf("Line not valid\n");
	else	
		printf("%lf:%s  %i x %i\n", time, interfaceName, resx, resy);
}

//Convert a string hex character into its integer value
char getValue(char c) {
	char toReturn = 0;
	if (c == 'A') return (char)10;
	if (c == 'B') return (char)11;
	if (c == 'C') return (char)12;
	if (c == 'D') return (char)13;
	if (c == 'E') return (char)14;
	if (c == 'F') return (char)15;
	//If it wasn't one of the letter hex values, then it's
	//a digit from 0-9
	char str[2];
	str[0] = c; str[1] = '\0';
	return (char)atoi(str);
}

//Convert two hex values into a byte
char getBinary(char c1, char c2) {
	return (char)(16 * getValue(c1) + getValue(c2));
}

void writeJPEG(FILE* fin, FILE* fout) {
	//Write the hex string out byte by byte
	char c1;
	char c2;
	fgetc(fin);//Get rid of the first space
	do {
		c1 = fgetc(fin);
		c2 = fgetc(fin);
		fprintf(fout, "%c", getBinary(c1, c2));
		//printf("%2X  %c%c\n", getBinary(c1, c2), c1, c2);
	}
	while (c1 != '\n' && c1 >= 0 && c2 != '\n' && c2 >= 0);
}

int main(int argc, char** argv) {
	printf("NOTE: There MUST be a directory called \"capture\"\n");
	int counter = 0;
	char filename[128];
	char command[256];
	struct PlayerHeader header;
	if (argc < 3) {
		fprintf(stderr, "syntax: log2jpeg <logfile> <jpegbasename>");
		return 1;
	}
	FILE* fin = fopen(argv[1], "r");
	fin = fopen(argv[1], "r");
	if (fin == NULL) {
		fprintf(stderr, "ERROR opening input file %s\n", argv[1]);
		return 1;
	}
	while (!feof(fin)) {
		header.readHeader(fin);
		if (header.valid) {
			sprintf(filename, "capture/%s%i.jpg", argv[2], counter);
			FILE* fout = fopen(filename, "w");
			writeJPEG(fin, fout);
			counter++;
		}
	}
	//Delete all of the hold string hex values in the log file since they've now all
	//been written to jpeg files (this saves space)
	//(this is done the dirty and nasty way by invoking system commands)
	sprintf(command, "colrm 16 1000000 < %s > out", argv[1]);
	system(command);
	sprintf(command, "rm %s", argv[1]);
	system(command);
	sprintf(command, "cp out %s", argv[1]);
	system(command);
	system("rm out");
	return 0;
}
