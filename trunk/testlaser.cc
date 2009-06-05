//g++ -o testlaser $(pkg-config --cflags --libs playerc++) testlaser.cc

#include </usr/local/include/player-2.2/libplayerc++/playerc++.h>
#include <iostream>

#include "args.h"

#define RAYS 32
#define PI 3.141
#define MIN_ANGLE -PI/2
#define MAX_ANGLE PI/2
#define ANGLE_SCANRES_DEGREES 1
#define RANGE_RES_MM 10
#define FREQUENCY 10.0

int main(int argc, char *argv[]) {
	parse_args(argc, argv);
	bool INTENSITY = false;
	double speed = 1.5;
	double critical_min_dist = 0.5;

	try
	{
		using namespace PlayerCc;

		PlayerClient robot(gHostname, gPort);
		Position2dProxy pp(&robot, gIndex);
		LaserProxy laser(&robot, gIndex);

		laser.Configure(MIN_ANGLE, MAX_ANGLE, 100 * ANGLE_SCANRES_DEGREES,
				RANGE_RES_MM, INTENSITY, FREQUENCY);
		laser.RequestConfigure();
		int bins = laser.GetCount();
		while (true) {
			robot.Read();
			bins = laser.GetCount();
			double maxrange = laser.GetMaxRange();
			if (bins <= 0)
				continue;
			int minindex = 0;
			double minvalue = maxrange;
			int maxindex = 0;
			double maxvalue = 0;
			for (int i = 0; i < bins; i++) {
				if (laser.GetRange(i) < minvalue) {
					minvalue = laser.GetRange(i);
					minindex = i;
				}
				if (laser.GetRange(i) > maxvalue) {
					maxvalue = laser.GetRange(i);
					maxindex = i;
				}
			}
			double minangle = -(minindex - bins/2) / (double)bins * MIN_ANGLE * 2.0;
			double maxangle =  -(maxindex - bins/2) / (double)bins * MAX_ANGLE * 2.0;
			double angle = -minangle;
			//If the robot is close to an obstacle, try to turn away from that obstacle
			if (minvalue < critical_min_dist) {
				angle = ((angle < 0)?-1:1) * PI/2 - angle;
			}
			if (abs(angle) > PI/2 || minvalue > critical_min_dist) {
				//If the closest thing makes at least a right
				//angle with the way the robot is facing
				//Check to see if the front path is reasonably clear
				if (laser.GetRange(bins / 2) > 1.0)
					angle = 0;//Go straight ahead if it's clear
			}
			printf("minvalue = %f, bins = %i, minindex = %i, angle = %f \n", minvalue, bins, minindex, angle);
			pp.SetSpeed(speed * minvalue / maxrange, angle);
		}

	}
	catch (PlayerCc::PlayerError e)
	{
		std::cerr << e << std::endl;
		return -1;
	}
	return 0;
}

