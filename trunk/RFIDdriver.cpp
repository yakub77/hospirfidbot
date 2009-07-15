/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003  
 *     Brian Gerkey
 *                      
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * A simple example of how to write a driver that will be built as a
 * shared object.
 */

// ONLY if you need something that was #define'd as a result of configure 
// (e.g., HAVE_CFMAKERAW), then #include <config.h>, like so:
/*
#if HAVE_CONFIG_H
  #include <config.h>
#endif
*/

#include <unistd.h>
#include <string.h>

#include <libplayercore/playercore.h>

#include "RFIDdriver.h"

////////////////////////////////////////////////////////////////////////////////
// The class for the driver


// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver* 
RFIDdriver_Init(ConfigFile* cf, int section) {
	// Create and return a new instance of this driver
	return((Driver*)(new RFIDdriver(cf, section)));
}

// A driver registration function, again declared outside of the class so
// that it can be invoked without object context.  In this function, we add
// the driver into the given driver table, indicating which interface the
// driver can support and how to create a driver instance.
void RFIDdriver_Register(DriverTable* table) {
	table->AddDriver("rfiddriver", RFIDdriver_Init);
}

////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
RFIDdriver::RFIDdriver(ConfigFile* cf, int section)
    : Driver(cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, 
             PLAYER_RFID_CODE) {
	// Read an option from the configuration file
	this->baudrate = cf->ReadInt(section, "baudrate", DEFAULT_BAUD);
	this->port = (char*)cf->ReadString(section, "port", "/dev/ttyUSB0");
	
	printf("baudrate = %i\nport = %s\n", this->baudrate, this->port); 
	return;
}

int RFIDdriver::Connect (int port_speed) {
	// Open serial port
	fd = open (port, O_RDWR);
	if (fd < 0)
	{
	PLAYER_ERROR2 ("> Connecting to RFID Device on [%s]; [%s]...[failed!]",
		   (char*) port, strerror (errno));
	return (-1);
	}

	// Change port settings
	struct termios options;
	memset (&options, 0, sizeof (options));// clear the struct for new port settings
	// Get the current port settings
	if (tcgetattr (fd, &options) != 0) {
	PLAYER_ERROR (">> Unable to get serial port attributes !");
	return (-1);
	}
	tcgetattr (fd, &initial_options);

	// turn off break sig, cr->nl, parity off, 8 bit strip, flow control
	options.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

	// turn off echo, canonical mode, extended processing, signals
	options.c_lflag &= ~(ECHO | ECHOE | ICANON | IEXTEN | ISIG);

	options.c_cflag &= ~(CSTOPB);   // use one stop bit
	options.c_cflag &= ~(PARENB);   // no parity
	options.c_cflag &= ~(CSIZE );   // clear size
	options.c_cflag |= (CS8);     // set bit size (default is 8)
	options.c_oflag &= ~(OPOST);  // turn output processing off

	// read satisfied if TIME is exceeded (t = TIME *0.1 s)
	//  options.c_cc[VTIME] = 1;
	//  options.c_cc[VMIN] = 0;

	// Change the baud rate
	switch (port_speed) {
		case 1200: baudrate = B1200; break;
		case 2400: baudrate = B2400; break;
		case 4800: baudrate = B4800; break;
		case 9600: baudrate = B9600; break;
		case 19200: baudrate = B19200; break;
		case 38400: baudrate = B38400; break;
		case 57600: baudrate = B57600; break;
		case 115200: baudrate = B115200; break;
		default: {
			PLAYER_ERROR1 (">> Unsupported speed [%d] given!", port_speed);
			return (-1);
		}
	}
	// Set the baudrate to the given port_speed
	cfsetispeed (&options, baudrate);
	cfsetospeed (&options, baudrate);

	// Activate the settings for the port
	if (tcsetattr (fd, TCSAFLUSH, &options) < 0)
	{
	PLAYER_ERROR (">> Unable to set serial port attributes !");
	return (-1);
	}

	PLAYER_MSG1 (1, "> Connecting to SICK RFI341 at %dbps...[done]", port_speed);
	// Make sure queues are empty before we begin
	tcflush (fd, TCIOFLUSH);

	return (0);
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int RFIDdriver::Setup() {   
	puts("new RFID driver initialising");

	// Here you do whatever is necessary to setup the device, like open and
	// configure a serial port.

	Connect(baudrate);

	puts("Example driver ready");

	// Start the device thread; spawns a new thread and executes
	// RFIDdriver::Main(), which contains the main loop for the driver.
	StartThread();

	return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
int RFIDdriver::Shutdown() {
	puts("Shutting example driver down");

	// Stop and join the driver thread
	StopThread();

	// Here you would shut the device down by, for example, closing a
	// serial port.

	puts("Example driver has been shutdown");

	return(0);
}

int RFIDdriver::ProcessMessage(QueuePointer & resp_queue, 
                                  player_msghdr * hdr,
                                  void * data) {
	// Process messages here.  Send a response if necessary, using Publish().
	// If you handle the message successfully, return 0.  Otherwise,
	// return -1, and a NACK will be sent for you, if a response is required.
	return(0);
}



void sendSerial(unsigned char* message, int length) {
	write(fd, val, length);
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void RFIDdriver::Main() {
    while (true)  {
	// Check to see if Player is trying to terminate the plugin thread
	pthread_testcancel();

	// Process messages
	ProcessMessages(); 

        /*player_rfid_data_t data_rfid;
        data_rfid.tags = new player_rfid_tag_t[1];
        
        //TODO:

        //Publishing data.
        if (rfid_id.interf !=0) {
            Publish(rfid_id, PLAYER_MSGTYPE_DATA, PLAYER_RFID_DATA_TAGS, (unsigned char*)&data_rfid, sizeof(player_rfid_data_t), NULL);
        }
        delete [] data_rfid.tags[0].guid;
        delete [] data_rfid.tags;*/
    }
}

////////////////////////////////////////////////////////////////////////////////
// Extra stuff for building a shared object.

/* need the extern to avoid C++ name-mangling  */
extern "C" {
	int player_driver_init(DriverTable* table) {
		puts("New RFID driver initializing");
		RFIDdriver_Register(table);
		puts("New RFID driver done");
		return(0);
	}
}

