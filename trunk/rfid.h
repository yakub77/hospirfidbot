/*Author: Chris Tralie
 *Project: Duke REU Fellowship 2009: Robotic navigation with RFID Waypoints
 *Purpose: To create an object that stores all of the information from an
 *RFID tag log (in the format that Travis Deyle and I use) in an orderly fashion.*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <map>
#include <vector>
#include <string>
#include "comparators.h"


#ifndef RFID_H
#define RFID_H

using namespace std;

struct TagReading {
	int id, strength;
};

typedef struct TagReading tread;
typedef map<string, int> simap;
typedef map<int, string> ismap;

class RFIDLog {
public:
	RFIDLog();
	RFIDLog(const char* filename);
	~RFIDLog();
    unsigned int lines;
    unsigned int numtags;
    simap tags;//Used to store all of the unique tag IDs;
    //convert from hex string to an integer id that I assign
    
    ismap reverseTags;//Store the same information with the role of
    //the key and value switched (this will sort the tags in the order
    //that they were seen)
    
    //Given a time, look up all of the RFID readings at that time in
    //the log
	map<double, vector<tread>, CompareDouble> timeReadings;
	vector<tread> getClosestReadings(double time);
};

#endif
