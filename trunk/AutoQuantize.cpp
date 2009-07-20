#include "pgm.h"

int main(int argc, char** argv) {
	PGMImage map("logs/fine.pgm");
	map.autoQuantize();
	map.writeToFile("logs/map.pgm");
	return 0;
}
