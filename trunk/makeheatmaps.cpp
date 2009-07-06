#include "pgm.h"
#include "rfid.h"
#include "heatmap.h"

int main(int argc, char** argv) {
	//HeatMaps(double mres, const char* mapfile, const char* logfileamcl, const char* logfilerfid);
	HeatMaps heatmaps(0.05, "logs/map.pgm", "logs/localized.log", "logs/rfidtags.log");
	simap::iterator iter = heatmaps.rfidlog->tags.begin();
	char filename[128];
	printf("There are %i tags\n", heatmaps.rfidlog->tags.size());
	while (iter != heatmaps.rfidlog->tags.end()) {
		string name = iter->first;
		int id = iter->second;
		printf("%s -> %i\n", name.data(), id);
		sprintf(filename, "heatmap%i.pgm", id);
		heatmaps.saveHeatMap(id, (const char*)filename);
		iter++;
	}
	return 0;
}
