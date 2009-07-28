/*Author: Chris Tralie
 *Project: Duke REU Fellowship 2009: Robotic navigation with RFID Waypoints
 *Purpose: A program that can navigate down a hallway using laser scans and 
 *sending commands to the motors.  The program rasterizes the laser scans 
 *into a 2D rectangular grid of occupied or unoccupied cells.  
 *It then calculates the centroid of these cells and steers towards that.  
 *This makes it follow the center of the hallway.*/

#include <libplayerc++/playerc++.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <ctime>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#include "args.h"
#include "speak.h"

#define PI 3.141

using namespace PlayerCc;

class Point {
public:
	Point();
	Point(double px, double py);
	double x, y;
	//Assignment operator
	Point& operator=(const Point& point);
	Point& operator+=(const Point& point);
	Point& operator*=(const double a);

	double getAngle();
	double getSquaredMag();
	double getMag();
	void print();
};

Point::Point() {
	x = 0.0; y = 0.0;
}

Point::Point(double px, double py) {
	x = px; y = py;
}

Point& Point::operator=(const Point& point) {
	x = point.x;y=point.y;
	return *this;
}

Point& Point::operator+=(const Point& point) {
	x += point.x;y += point.y;
	return *this;
}

Point& Point::operator*=(const double a) {
	x = x * a; y = y * a;
	return *this;
}

double Point::getAngle() {
	//Rotate the angle by 90 degrees, so that zero degrees occurs when x = 0
	//a negative angle occurs when x < 0, and a positive angle occurs when x > 0
	//Basically, we want to figure out the angle that the point makes with the forward direction
	return atan(-x / y);
}

double Point::getSquaredMag() {
	return x*x + y*y;
}

double Point::getMag() {
	return sqrt(x*x + y*y);
}

void Point::print() {
	printf("(%f, %f)\n", x, y);
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


class OccupancyGrid {
public:
	double increment, owidth, oheight;//in meters
	int oarrwidth, oarrheight;//Width and heigh of occupancy grid (in increment units)	
	bool** grid;
	OccupancyGrid(double inc, double width, double height);
	~OccupancyGrid();
	void fill(LaserProxy* laser, int bins, double MIN_ANGLE, double MAX_ANGLE);
	void output();
	Point GetCoord(int x, int y);
	Point GetCentroid();
	Point GetRightBiasCentroid(double centerX);
};

OccupancyGrid::OccupancyGrid(double inc, double width, double height) {
	increment = inc;
	owidth = width;
	oheight = height;
	oarrwidth = (int)(owidth / increment);
	oarrheight = (int)(oheight / increment);
	grid = (bool**)calloc(oarrwidth, sizeof(bool**));
	for (int x = 0; x < oarrwidth; x++) {
		grid[x] = (bool*)calloc(oarrheight, sizeof(bool*));
	}
}

OccupancyGrid::~OccupancyGrid() {
	for (int i = 0; i < oarrwidth; i++) {
		free(grid[i]);
	}
	free(grid);
}

void OccupancyGrid::fill(LaserProxy* laser, int bins, double MIN_ANGLE, double MAX_ANGLE) {
	double inc = (MAX_ANGLE - MIN_ANGLE) / (double)bins;
	for (int x = 0; x < oarrwidth; x++) {
		double dx = increment * (double)(x - oarrwidth / 2);
		for (int y = 0; y < oarrheight; y++) {
			double dy = increment * (double)y;
			Point point(dx, dy);
			double angle = point.getAngle();
			double sqrdist = point.getSquaredMag();
			//Find the nearest bin in the laser scan (converting from rectangular to polar)
			//NOTE: This is the simple approach for now, but could cause aliasing
			int bin = (int)(angle / inc) + bins / 2;
			if (laser->GetRange(bin)*laser->GetRange(bin) > sqrdist)
				grid[x][y] = false;//The cell is empty
			else
				grid[x][y] = true;//The cell is full
		}
	}
}


//Output grid to ASCII file
void OccupancyGrid::output() {
	FILE* file = fopen("grid.txt", "w");
	for (int x = 0; x < oarrwidth; x++) {
		for (int y = 0; y < oarrheight; y++) {
			if (grid[x][y]) fprintf(file, "* ");//Obstacle in the way
			else fprintf(file, "- ");//Open space
		}
		fprintf(file, "\n");
	}
	fclose(file);
}

//Convert from array index to position offset in meters
Point OccupancyGrid::GetCoord(int x, int y) {
	Point coord;
	coord.x = (double)(x - oarrwidth / 2) * increment;
	coord.y = (double)y * increment;
	return coord;
}

//Get the centroid of the open area
Point OccupancyGrid::GetCentroid() {
	Point toReturn;
	double total = 0.0;
	for (int x = 0; x < oarrwidth; x++) {
		for (int y = 0; y < oarrheight; y++) {
			if (!grid[x][y]) {  //If the cell is empty, weight it in the centroid
				toReturn += GetCoord(x, y);
				total++;			
			}
		}
	}
	toReturn.x /= total; toReturn.y /= total;
	return toReturn;
}

Point OccupancyGrid::GetRightBiasCentroid(double centerX) {
	Point toReturn;
	double total = 0.0;
	for (int x = 0; x < oarrwidth; x++) {
		for (int y = 0; y < oarrheight; y++) {
			if (!grid[x][y]) {  //If the cell is empty, weight it in the centroid
				Point coord = GetCoord(x, y);								
				if (coord.x >= centerX) {				
					toReturn += coord; total ++;
				}		
			}
		}
	}
	toReturn.x /= total; toReturn.y /= total;
	return toReturn;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
	parse_args(argc, argv);
	bool INTENSITY = false;
	double cruising_speed = 0.25;
	double critical_min_dist = 0.5;
	OccupancyGrid grid(0.1, 4.0, 2.0);
	bool rightbias = false;//Drive towards the right side of the hallway?

	//speak("This is test speech");	

	try {
	PlayerClient robot(gHostname, gPort);
	Position2dProxy pp(&robot, gIndex);
	LaserProxy laser(&robot, gIndex);

	while (true) {
		robot.Read();
		int bins = laser.GetCount();
		//printf("%i\n", bins);
		double MIN_ANGLE = laser.GetMinAngle();
		double MAX_ANGLE = laser.GetMaxAngle();
		double maxrange = laser.GetMaxRange();
		if (bins <= 0)
			continue;
		int minindex = 0;
		double minvalue = maxrange;
		for (int i = 0; i < bins; i++) {
			if (laser.GetRange(i) < minvalue) {
				minvalue = laser.GetRange(i);
				minindex = i;
			}
		}
		double minangle = -(minindex - bins/2) / (double)bins * MIN_ANGLE * 2.0;
		grid.fill(&laser, bins, MIN_ANGLE, MAX_ANGLE);
		//grid.output();
		Point centroid = grid.GetCentroid();
		if (rightbias) 
			centroid = grid.GetRightBiasCentroid(centroid.x);
		double angle = centroid.getAngle();

		//Uncomment this to make the robot halt when it gets too close to
		//something and say "please move out of the way" until the obstruction
		//is cleared
		/*(minvalue < critical_min_dist && fabs(minangle)) {			
			//Something's in the way
			pp.SetSpeed(0.0, 0.0);
			speak("Please move out of the way");		
			sleep(0.5);
		}
		else {	*/	
			//Slow down to make turns
			double speed = cruising_speed;
			speed /= (1.0 + 5.0 * fabs(angle));
			//printf("angle=%f, speed=%f\n", fabs(angle), speed);		
			pp.SetSpeed(speed, angle);
			//pp.SetSpeed(0.5, 0);
		//}
	}

	}
	catch (PlayerCc::PlayerError e)
	{
		std::cerr << e << std::endl;
		return -1;
	}
	return 0;
}

