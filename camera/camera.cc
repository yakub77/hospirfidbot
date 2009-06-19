#include <libplayerc++/playerc++.h>
#include "args.h"
#include <iostream>
#include <ctime>
#include <stdio.h>

int main(int argc, char** argv)
{
  parse_args(argc, argv);
  const char* names = "capture";
  FILE* file = fopen("out.txt", "w");
  int num = 0;

  try
  {
    PlayerCc::PlayerClient client(gHostname, gPort);
    PlayerCc::CameraProxy cp(&client, gIndex);

    while (true) {
      client.Read();
      cp.SaveFrame(names);
      fprintf(file, "%i %ld\n", num, time(NULL));
      fflush(file);
      printf("%i %ld\n", num, time(NULL));
      sleep(1);
      num++;
    }

  }
  catch (PlayerCc::PlayerError e)
  {
    std::cerr << e << std::endl;
    fclose(file);
    return -1;
  }
  fclose(file);
  return 1;
}
