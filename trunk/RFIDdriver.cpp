
#if HAVE_CONFIG_H
  #include <config.h>
#endif

#include <unistd.h>
#include <string.h>

#include <libplayercore/playercore.h>

#include "RFIDdriver.h"

static int TIMEOUT = 2;//Timeout in seconds
static bool debug = false;//Print out the hex data for sending / receiving commands
static bool printlive = false;//Controls whether all of the tag information is printed to stdout 
//in addition to the logfile

//This function updates a running CRC value
void CRC_calcCrc8(u16* crc_calc, u8 ch) {
	u8 i, v, xor_flag;
	
	//Align test bit with leftmost bit of the MsgObj byte
	v = 0x80;
	for (int i = 0; i < 8; i++) {
		if (*crc_calc & 0x8000)
			xor_flag = 1;
		else
			xor_flag = 0;
		*crc_calc = *crc_calc << 1;
		if (ch & v)
			*crc_calc = *crc_calc + 1;
		if (xor_flag)
			*crc_calc = *crc_calc ^ MSG_CCITT_CRC_POLY;
		//Align test bit with next bit of the MsgObj byte.  
		v = v >> 1;
	}
}

u16 CRC_calcCrcMsgObj(MsgObj* MsgObj) {
	u16 crcReg, i;
	crcReg = MSG_CRC_INIT;
	
	CRC_calcCrc8(&crcReg, MsgObj->dataLen);
	CRC_calcCrc8(&crcReg, MsgObj->opCode);
	for (int i = 0; i < MsgObj->dataLen; i++) {
		CRC_calcCrc8(&crcReg, MsgObj->data[i]);
	}
	return crcReg;
}

//Used to validate the CRC for the received bytes
bool validateCRC(u8* buf, int n) {
	//TODO: Fix this (it doesn't work right now)
	if (n < 7)
		return false;
	u16 crcReg = MSG_CRC_INIT;
	for (int i = 0; i < n; i++) {
		CRC_calcCrc8(&crcReg, buf[i]);
	}
	//This is the CRC returned from the reader
	u16 thiscrc = ((buf[n - 1] & 0x00FF) << 8) | (buf[n - 2] & 0x00FF);
	printf("%x %x", buf[n -2], buf[n - 1]);
	bool equal = (thiscrc == crcReg);
	if (!equal) {
		fprintf(stderr, "Warning: CRC check failed: Got %x, expecting %x\n", thiscrc, crcReg);
	}
	return equal;
}

MsgObj::MsgObj(u8 dL, u8 oC, u8* d) {
	dataLen = dL;
	opCode = oC;
	for (int i = 0; i < dL; i++)
		data[i] = d[i];
	length = getLength();
}


//[header 1 byte] [data length 1 byte] [command 1 byte] [status word 2 bytes] [data M bytes] [crc 2 bytes]
MsgObj::MsgObj(u8* bytes, int len) {
	assert(len >= 7);
	if (bytes[0] != 0xFF) fprintf(stderr, "ERROR: header byte (%x) is not 0xFF\n", bytes[0]);
	dataLen = bytes[1];
	opCode = bytes[2];//OpCode of the last command received;
	status = ((bytes[3] << 8) & 0xFF00) | (0x00FF & bytes[4]);
	for (u8 i = 0; i < dataLen; i++) {
		data[i] = bytes[i + 5];
	}
}

MsgObj::~MsgObj() {

}

int MsgObj::getLength() {
	return (int)dataLen + 5;
}

void MsgObj::getBytesToSend(u8* bytes) {
	bytes[0] = 0xFF;
	bytes[1] = dataLen;
	bytes[2] = opCode;
	for (int i = 3; i < length && (i - 3) < dataLen; i++) {
		bytes[i] = data[i - 3];
	}
	u16 crc = CRC_calcCrcMsgObj(this);
	u8 lo = (u8)(crc & 0x00FF);
	u8 hi = (u8)((crc & 0xFF00) >> 8);
	bytes[length - 2] = hi;
	bytes[length - 1] = lo;
}



/////////////////////////////////////////////////////////////////////

Driver* 
RFIDdriver_Init(ConfigFile* cf, int section) {
	// Create and return a new instance of this driver
	return((Driver*)(new RFIDdriver(cf, section)));
}

void RFIDdriver_Register(DriverTable* table) {
	table->AddDriver("rfiddriver", RFIDdriver_Init);
}

RFIDdriver::RFIDdriver(ConfigFile* cf, int section)
    : Driver(cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, 
             PLAYER_RFID_CODE) {
	this->port = (char*)cf->ReadString(section, "port", "/dev/ttyUSB0");
	debug = atoi((char*)cf->ReadString(section, "debug", "0"));
	printlive = atoi((char*)cf->ReadString(section, "printlive", "0"));
	
	readPwr = 3000;
	//Let the user specify the location of the logfile
	logfile = fopen((char*)cf->ReadString(section, "logfile", "rfidtags.log"), "w");
	return;
}

int RFIDdriver::Connect (int port_speed) {
	// Open serial port
	fd = open (port, O_RDWR);
	if (fd < 0) {
		PLAYER_ERROR2 ("> Connecting to RFID Device on [%s]; [%s]...[failed!]",
			   (char*) port, strerror (errno));
		return -1;
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


	options.c_cc[VMIN] = 0;
	options.c_cc[VTIME] = 10 * TIMEOUT;//Have a timeout of 2 seconds

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
		case 230400: baudrate = B230400; break;
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
	if (tcsetattr (fd, TCSAFLUSH, &options) < 0) {
		PLAYER_ERROR (">> Unable to set serial port attributes !");
		return (-1);
	}

	//PLAYER_MSG1 (1, "> Connecting to RFID Reader at %dbps...[done]", port_speed);
	// Make sure queues are empty before we begin
	tcflush (fd, TCIOFLUSH);

	return (0);
}

void RFIDdriver::Disconnect() {
	close(fd);
}

//Pass the command and data parts of the MsgObj here
void RFIDdriver::sendMessage(u8 command, u8* data, int length) {
	MsgObj message(length, command, data);
	u8 bytes[256];
	message.getBytesToSend(bytes);
	if (debug) {
		printf("\n\nSending hex: ");
		for (int i = 0; i < message.getLength(); i++)
			printf("%x ", bytes[i]);
		printf("\n\n");
	}
	write(fd, bytes, message.getLength());
	tcflush(fd, TCIOFLUSH);
}

//[SOH 1 byte] [length 1 byte] [op code 1 byte] [status 2 bytes] [data n bytes] [CRC 2 bytes]
int RFIDdriver::readMessage(u8* data, int length, int timeout) {
	memset(data, 0x00, length);//Clear the previous contents from the buffer
	int minlength = 7;
	int n = 0;
	time_t start = time(NULL);
	while (n < minlength) {
		int elapsed = (int)(time(NULL) - start);
		int k = read(fd, &data[n], length);
		n += k;
		if (k == 0 && elapsed >= timeout) break;//At the point when no data is left, stop reading it
		//(this seems to work, and it's faster than waiting for the timeout timer
		//which I commented out above)
		if (n >= 2) {
			minlength = 7 + data[1];//Add in the length of the data;
		}
	}
	if (n == 0) 
		fprintf(stderr, "Error: Serial communication timed out\n");
	else if (debug) {
		printf("\n\nReceived hex length %i (supposed to be %i): ", n, minlength);
		for (int i = 0; i < n; i++)
			printf("%x ", data[i]);
		printf("\n\n");
	}
	//validateCRC(data, n);
	return n;
}


//Return true if the message has the 0x0000 code for completed successfully
bool checkSuccess(u8* buf, int n) {
	if (n < 7) {
		return false;
	}
	MsgObj received(buf, n);
	bool toReturn = (received.status == 0x0000);
	if (!toReturn) {
		fprintf(stderr, "Warning: Did not receive 0x0000 acknowledge code; got %x instead\n", received.status);
	}
	return toReturn;
}

//Return true if successful
bool RFIDdriver::checkBootFirmwareVersion() {
	u8 buf[256];
	sendMessage(0x03, NULL, 0);//"Get bootloader firmware version number"
	int n = readMessage(buf, 256, 0);
	return checkSuccess(buf, n);
}

bool RFIDdriver::bootFirmware() {
	u8 buf[256];
	
        //Check if BootLoader is running by issuing "Get BootLoader/Firmware Version"        
        //if (!checkBootFirmwareVersion()) 
        //	return false;
        
        //Boot into Firmware
        printf("\tBooting into firmware\n");
        sendMessage(0x04, NULL, 0);
	
	int n = readMessage(buf, 256, 0);
	if (n < 7) return false;
	
	MsgObj response(buf, 256);
	
	if (response.status == 0x0000)
		return true;//New boot was successful

	// Non-Zero Response will be received if the reader has already booted into firmware
	//   This occurs when you've already powered-up & previously configured the reader.  
	//   Can safely ignore this problem and continue initialization
        if (response.status == 0x0101) //This actually means "invalid opcode"
		return true;
		
	return false;
}

bool RFIDdriver::ChangeAntennaPorts(u8 TXport, u8 RXport) {
	u8 buf[256];
	u8 data[2] = {TXport, RXport};
	sendMessage(0x91, data, 2);
	int n = readMessage(buf, 256, 0);
	return checkSuccess(buf, n);
	
}
        
bool RFIDdriver::ChangeTXReadPower(u16 r) {
        u8 buf[256];
        readPwr = r;
        u8 hi = (readPwr & 0xFFFF) >> 8;
        u8 lo = (readPwr & 0x00FF);
        u8 data[2] = {hi, lo};
        sendMessage(0x92, data, 2);
        int n = readMessage(buf, 256, 0);
        return checkSuccess(buf, n);
}

//Set protocol to gen2
bool RFIDdriver::setProtocol() {
        u8 buf[256];
        u8 data[2] = {0x00, 0x05};
        sendMessage(0x93, data, 2);
        int n = readMessage(buf, 256, 0);
        return checkSuccess(buf, n);
}

bool RFIDdriver::setRegion() {
        //Set Region (we're only going to deal with North America)
        u8 buf[256];
        u8 data[1] = {0x01};
        sendMessage(0x97, data, 1);
        int n = readMessage(buf, 256, 0);
        return checkSuccess(buf, n);
}

void RFIDdriver::QueryEnvironment(u16 timeout) {
	u8 buf[256];

        //Send "Read Tag ID Multiple" Command (opCode 22)
        u8 timeoutHi = (timeout & 0xFFFF) >> 8;
        u8 timeoutLo = timeout & 0x00FF;
        
        u8 readmultipledata[4] = {0x00, 0x00, timeoutHi, timeoutLo};
        sendMessage(0x22, readmultipledata, 4);
	
	timeval tim;
	gettimeofday(&tim, NULL);
	double readtime = tim.tv_sec + (tim.tv_usec / 1000000.0);
	
	usleep(timeout * 2);//Sleep to give the reader enough time to execute this command and 
	//send the data back
	int n = readMessage(buf, 256, 0);
	if (n < 7) {
  		fprintf(stderr, "ERROR sending \"Read Tag ID Multiple\"; n = %i\n", n);	
  		return;
	}
	MsgObj received(buf, n);
	if (received.status == 0x0400) {
  		//No tags were found
  		return;
	}
	
	//At least one tag was seen
	sendMessage(0x29, NULL, 0);
	n = readMessage(buf, 256, 0);
	if (n < 11) {
		fprintf(stderr, "ERROR sending Get Tag Buffer command\n");
		return;
	}
	
	MsgObj* tagread = new MsgObj(buf, n);
	u16 ReadIndex = (tagread->data[0] << 8) & 0xFF00 | (tagread->data[1] & 0x00FF);
	u16 WriteIndex = (tagread->data[2] << 8) & 0xFF00 | (tagread->data[3] & 0x00FF);
	delete tagread;
	
	int numTags = WriteIndex - ReadIndex;
	
	if (printlive)	printf("%.2lf %i ", readtime, numTags);
	fprintf(logfile, "%.2lf %i ", readtime, numTags);
	
	while (numTags > 0) {
		u8 getbufferdata[3] = {0x00, 0x02, 0x00};
		sendMessage(0x29, getbufferdata, 3);
		n = readMessage(buf, 256, 0);
		if (n < 7) {
			fprintf(stderr, "ERROR: Reading buffer failed\n");
			break;
		}
		tagread = new MsgObj(buf, n);
		if (tagread->dataLen < 4) {
			fprintf(stderr, "ERROR: Reading buffer failed\n");
			break;		
		}
		u8 num = tagread->data[3];
		for (u8 i = 0; i < num; i++) {
			int rssi = tagread->data[4 + i*19];
			//Print the tag id as a hex string
			for (int j = 4 + i*19 + 5; j < 4 + i*19 + 5 + 12; j++) {
				if (printlive)	printf("%.2x", tagread->data[j]);
				fprintf(logfile, "%.2x", tagread->data[j]);
			}
			//Then print the tag strength
			if (printlive)	printf(" %i ", rssi);
			fprintf(logfile, " %i ", rssi);
		}
		if (printlive)	printf("\n");
		fprintf(logfile, "\n");
		delete tagread;
		numTags -= num;
	}
	
	
	//Reset and clear the Tag ID Buffer for next Read Tag ID Multiple
	sendMessage(0x2A, NULL, 0);
	n = readMessage(buf, 256, 0);
	if (!checkSuccess(buf, n)) {
		fprintf(stderr, "ERROR clearing buffer for next tag read");
	}
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int RFIDdriver::Setup() {   
	printf("RFID driver initialising\n\n");
	printf("\tAttempting 230400 bps...\n");
	Connect(230400);
	u8 buf[256];
	readMessage(buf, 256, 0);//Read out any junk that's left

	if (!checkBootFirmwareVersion())  {
		fprintf(stderr, "\tFailed @ 230400 bps\nAttempting 9600 bps...\n");
		Disconnect();
		Connect(9600);
		
		if (!checkBootFirmwareVersion()) {
			fprintf(stderr, "\tFailed @ 9600 bps\n");
			fprintf(stderr, "\tCould not open serial port %s at baudrate 230400 bps or 9600 bps\n", port);
			Disconnect();
			return 1;
		}
		else {
			printf("\tSuccessful @ 9600 bps\n");
			//Change baud to 230400 bps
			printf("\tSwitching to 230400 bps\n");
			u8 newbaudrate[4] = {0x00, 0x03, 0x84, 0x00};
			sendMessage(0x06, newbaudrate, 4);
			int n = readMessage(buf, 256, 0);
			checkSuccess(buf, n);
			Disconnect();
			printf("\tAttempting to reconnect @ 230400 bps\n");
			Connect(230400);
			
			if (!checkBootFirmwareVersion()) {
				fprintf(stderr, "\tFailed @ 230400 bps\n");
				return 1;	
			}
			else
				printf("\tSuccessfuly reconnected @ 230400 bps\n");
			
		}
	}
	else {
		printf("\tSuccessful @ 230400 bps\n");
	}
	
	//Now boot into firmware
	if (!bootFirmware()) {
		fprintf(stderr, "Failed to boot firmware\n");
		return 1;
	}

	if (!ChangeAntennaPorts(1, 1)) {
		fprintf(stderr, "Failed to change antenna ports\n");
		return 1;
	}
	
	if (!ChangeTXReadPower(readPwr)) {
		fprintf(stderr, "Failed to change read power to %i\n", readPwr);
		return 1;
	}
	
	if (!setProtocol()) {
		fprintf(stderr, "Failed to set protocol to GEN2\n");
		return 1;
	}
	
	if (!setRegion()) {
		fprintf(stderr, "Failed to set region\n");
		return 1;
	}

	// Set Power Mode (we'll just use default of "full power mode").
        // Use remaining defaults

	// Start the device thread; spawns a new thread and executes
	// RFIDdriver::Main(), which contains the main loop for the driver.
	StartThread();

	return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
int RFIDdriver::Shutdown() {
	// Stop and join the driver thread
	StopThread();
	return 0;
}

int RFIDdriver::ProcessMessage(QueuePointer & resp_queue, 
                                  player_msghdr * hdr,
                                  void * data) {
	// Process MsgObjs here.  Send a response if necessary, using Publish().
	// If you handle the MsgObj successfully, return 0.  Otherwise,
	// return -1, and a NACK will be sent for you, if a response is required.
	return(0);
}

////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void RFIDdriver::Main() {
    while (true)  {
		// Check to see if Player is trying to terminate the plugin thread
		pthread_testcancel();
	
		// Process MsgObjs
		ProcessMessages(); 
		
		QueryEnvironment(50);
		
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

