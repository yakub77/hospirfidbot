//g++ -o drive $(pkg-config --cflags --libs playerc++) drive.cc

#include </usr/local/include/player-2.2/libplayerc++/playerc++.h>
#include <iostream>

#include "args.h"

#define RAYS 32

int
main(int argc, char *argv[])
{
  parse_args(argc, argv);
  
  try
  {
    using namespace PlayerCc;
    
    PlayerClient robot(gHostname, gPort);
    Position2dProxy pp(&robot, gIndex);
    BumperProxy bumper(&robot, gIndex);
    
    

    pp.SetMotorEnable (true);
    
    double newspeed = 2;
      double newturnrate = 0;
      double minR = 1e9;
      double minL = 1e9;

    for(;;)
    {
      
      
        robot.Read();

	if (bumper.IsAnyBumped()) {
		pp.SetSpeed(-newspeed, 0);
		printf("BUMPED!!!\n");
		sleep(1.5);
		pp.SetSpeed(0, 0, newspeed);
		sleep(2);
	}      

 //     std::cout << "x=" << pp.GetXPos() << " y=" << pp.GetYPos() << " yaw=" << pp.GetYaw() << std::endl;
      
        
      
      pp.SetSpeed(newspeed, newturnrate);
    }
  }
    catch (PlayerCc::PlayerError e)
    {
      std::cerr << e << std::endl;
      return -1;
}
}
     
