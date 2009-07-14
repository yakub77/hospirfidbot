#ifndef RFIDDRIVER_H 
#define RFIDDRIVER_H

#include <unistd.h>
#include <string.h>

#include <libplayercore/playercore.h>
#include <termios.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#define DEFAULT_BAUD 9600

class RFIDdriver : public Driver {
public:
	// Constructor; need that
	RFIDdriver(ConfigFile* cf, int section);

	// Must implement the following methods.
	virtual int Setup();
	virtual int Shutdown();

	// This method will be invoked on each incoming message
	virtual int ProcessMessage(QueuePointer &resp_queue, 
		               player_msghdr * hdr,
		               void * data);

private:
	// Main function for device thread.
	virtual void Main();

	struct termios initial_options;
	int baudrate;
	char* port;
	player_devaddr_t rfid_id;
	int fd;
    	int Connect(int connect_speed);
};

#endif
