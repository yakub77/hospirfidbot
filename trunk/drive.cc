//g++ -o drive $(pkg-config --cflags --libs playerc++) drive.cc

#include <libplayerc++/playerc++.h>
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

    for(;;)
    {
	robot.Read();
	pp.SetSpeed(0.5, 0.0);
    }
  }
    catch (PlayerCc::PlayerError e)
    {
      std::cerr << e << std::endl;
      return -1;
}
}
     
