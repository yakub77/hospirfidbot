#include "rfid.h"

RFIDLog::RFIDLog(){}

//<time> <#tags> <string hex id> <strength 1> ..... 
RFIDLog::RFIDLog(const char* filename) {
	FILE* fp = fopen(filename, "r");
	double time;
	int tagsseen;
	if (fp == NULL) {
		fprintf(stderr, "ERROR: Unable to open file %s\n", filename);
		return;
	}
	numtags = 0;
	while (!feof(fp)) {
		vector<tread> readings;
		fscanf(fp, "%lf%i", &time, &tagsseen);
		for (int i = 0; i < tagsseen; i++) {
			char stringid[128];
			int strength;
			fscanf(fp, "%128s%i", stringid, &strength);
			//Check to see if this tag has already been spotted
			int tagid;
			if (tags.find(stringid) != tags.end()) {
				//printf("%s : %i \n", stringid, tagid);
				//The tag has already been encountered, so look up the integer
				//value it was previously assigned
				tagid = tags[stringid];
			}
			else {
				//It has not been found yet, so create a new entry for it
				tagid = numtags;
				tags[stringid] = tagid;
				numtags++;
			}
			struct TagReading reading;
			reading.id = tagid;
			reading.strength = strength;
			readings.push_back(reading);
		}
		timeReadings[time] = readings;
	}
	simap::iterator iter = tags.begin();
	while (iter != tags.end()) {
		reverseTags[iter->second] = iter->first;
		iter++;
	}
}

vector<tread> RFIDLog::getClosestReadings(double time) {
	assert(timeReadings.size() >= 1);
	//First check to see if the exact time is there (extremely unlikely)
	if (timeReadings.find(time) != timeReadings.end())
		return timeReadings[time];
	
	map<double, vector<tread>, CompareDouble>::iterator pos, before, after, closest;
	//The exact time is not there, so find the nearest one
	//Insert a fake entry to exploit the tree structure of the map;
	//the map is sorted already, so put in an entry and get a pointer to it
	//Then I can get a pointer to the time right before and the time right after
	//and figure out which one is closer
	
	vector<tread> dummy;
	vector<tread> toReturn;
	timeReadings[time] = dummy;
	pos = timeReadings.find(time);
	before = pos; before--;
	after = pos; after++;
	if (pos == timeReadings.begin()) {
		//Corner case: The time is less than the first time in the logfile
		toReturn = after->second;
	}
	else if (pos == timeReadings.end()) {
		//Corner case: The time is greater than the last time in the logfile
		toReturn = before->second;
	}
	else {
		//Normal case: The time is between two existing times in the logfile
		//Check to see if it's closer to the time before or the time after
		double dtb = time - before->first;
		double dta = after->first - time;
		if (dtb < dta)
			toReturn = before->second;
		else
			toReturn = after->second;
	}
	//printf("%f < %f < %f \n", before->first, time, after->first);
	
	//Clean up after adding the dummy value
	timeReadings.erase(pos);
	return toReturn;
}

RFIDLog::~RFIDLog() {}

//Test client
/*int main(int argc, char** argv) {
	RFIDLog log("logs/hallwayleft");
	vector<tread> readings = log.getClosestReadings(1245886250.6);
	for (int i = 0; i < readings.size(); i++) {
		printf("%i %i\n", readings[i].id, readings[i].strength);
	}
	return 0;
}*/
