#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef RFID_H
#define RFID_H


struct RFIDreads {
        char** tagName;
        int * sigStrength;
        int numRFID;
        void freeRead();
};

RFIDreads getRFIDreads(char* filename, float time);

#endif



