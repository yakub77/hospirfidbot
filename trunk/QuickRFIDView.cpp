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
