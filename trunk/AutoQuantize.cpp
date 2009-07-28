/*Author: Chris Tralie
 *Project: Duke REU Fellowship 2009: Robotic navigation with RFID Waypoints
 *Purpose: To provide access to the function that will clean up the occupancy grid
 *that player spits out and put it into a format that the AMCL driver can better understand*/


#include "pgm.h"

int main(int argc, char** argv) {
	PGMImage map("logs/fine.pgm");
	map.autoQuantize();
	map.writeToFile("logs/map.pgm");
	return 0;
}
