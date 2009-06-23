//g++ -o logdpslam $(pkg-config --cflags --libs playerc++) logdpslam.cc

#include <libplayerc++/playerc++.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include "args.h"

using namespace PlayerCc;

int main(int argc, char **argv) {
	parse_args(argc,argv);
	FILE* f = fopen(argv[1], "w");
	try {
		PlayerClient robot(gHostname, gPort);
		Position2dProxy pp(&robot, gIndex);
		LaserProxy laser(&robot, 1);
		while (true) {	
			robot.Read();
			int bins = laser.GetCount();
			//printf("%i\n", bins);
			//if (bins != 181) continue;
			double x = pp.GetXPos();
			double y = pp.GetYPos();
			double angle = pp.GetYaw();
			fprintf(f, "Odometry %lf %lf %lf\n", x, y, angle);
			fprintf(f, "Laser ");
			for (int i = 0; i < bins; i++) {
				fprintf(f, "%lf ", laser.GetRange(i));
			}
			fprintf(f, "\n");
			fflush(f);
		}
	}
	catch (PlayerCc::PlayerError e)
	{
		std::cerr << e << std::endl;
		return -1;
	}
}
