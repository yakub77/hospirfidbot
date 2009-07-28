/*Author: Chris Tralie
 *Project: Duke REU Fellowship 2009: Robotic navigation with RFID Waypoints
 *Purpose:  To create a driver in C++ (that's wrapped into the Player infrastructure)
 *that can communicate with the RFID reader we're using*/

#ifndef RFIDDRIVER_H 
#define RFIDDRIVER_H

#include <unistd.h>
#include <string.h>

#include <libplayercore/playercore.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctime>

#define DEFAULT_BAUD 9600

#define MSG_CRC_INIT 0xFFFF
#define MSG_CCITT_CRC_POLY 0x1021

typedef unsigned char u8;
typedef unsigned short u16;

//Make a class "MsgObj" that is able to split the streams of bytes
//into more sensible components
////[Header (1 byte)] [Data length (1 byte)] [Command (1 byte)] [Data* ] [CRC HI] [CRC LO]
class MsgObj {
public:
	MsgObj(u8 dL, u8 oC, u8* d);//Used for constructing a command to send to the reader
	MsgObj(u8* bytes, int len); //Used for constructing a command to parse from received bytes from the reader
	~MsgObj();
	u8 dataLen;
	u8 opCode;
	u8 data[256];
	u8 crc_high;
	u8 crc_low;
	u16 status;
	int length;
	
	int getLength();
	void getBytesToSend(u8* bytes);
};

class RFIDdriver : public Driver {
public:
	// Constructor; need that
	RFIDdriver(ConfigFile* cf, int section);

	// Must implement the following methods.
	virtual int Setup();
	virtual int Shutdown();

	// This method will be invoked on each incoming Message
	virtual int ProcessMessage(QueuePointer &resp_queue, 
		               player_msghdr * hdr,
		               void * data);

private:
	// Main function for device thread.
	virtual void Main();
	int Connect(int connect_speed);
	void Disconnect();
	void sendMessage(u8 command, u8* data, int length);
	int readMessage(u8* data, int length, int timeout);
	bool checkBootFirmwareVersion();
	bool bootFirmware();
	bool ChangeAntennaPorts(u8 TXport, u8 RXport);
	bool ChangeTXReadPower(u16 r);
	bool setProtocol();
	bool setRegion();
	void QueryEnvironment(u16 timeout);

	struct termios initial_options;
	char* port;//Port to connect with RFID reader
	player_devaddr_t rfid_id;
	int fd;//File descriptor for serial connection
	u16 readPwr;//set to 3000 in the main program
	FILE* logfile;//Logfile to log RFID readings along with
	//system time
	int baudrate;
	u16 querytimeout;//Timeout for reading tags
};

#endif
