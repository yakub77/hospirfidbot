/*Author: Chris Tralie
 *Project: Duke REU Fellowship 2009: Robotic navigation with RFID Waypoints
 *Purpose: To instantiate an RFID object for a given logfile, and to go through
 *and say what tags were seen in what order (and what their hex IDs were)*/

#include "rfid.h"
#include <iostream>
using namespace std;

int main(int argc, char** argv) {
	if (argc < 2) {
		cerr<< "Usage: QuickRFIDView <logfile>\n";
		return 1;
	}
	RFIDLog log(argv[1]);
	ismap::iterator iter = log.reverseTags.begin();
	while (iter != log.reverseTags.end()) {
		cout << iter->first << " -> " << iter->second << endl;
		iter++;
	}
	return 0;
}
