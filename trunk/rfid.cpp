//g++ -o rfid rfid.cpp
//To execute, type ./rfid



//This file converts the output log file of the RFID into a format which
//SLAM will be able to interpret


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream.h>

#define BUFSIZE 256
using namespace std;

void skipLine(FILE* fp) {
        char c;
        do {
                c = fgetc(fp);
        }
        while ((c >= 0) && (c != '\n')
}

struct RFIDHeader {
        bool valid;
        unsigned int lines;
        unsigned int numTags;
        double * time;
//        (large size) rfidName[BUFSIZE];
//        unsigned int sigStrength;
        
        void readHeader(FILE* fp);
        void Print();
};

struct RFIDreads {
        int * sigStrength;
        char** tagName;
        int numRFID;
};

void RFIDHeader::readHeader(FILE* fp) {
        char buf[BUFSIZE];
        time = new double[lines];
        
        for(int i=0; i<lines; i++) {
                fscanf(fp, "%20s", buf)
                if (buf[0] == '#') {
                        // A comment is encountered, assume this is not a valid header
                        // Skip everything in a line that's started with a comment
                        skipLine(fp);
                        valid = false;
                        return;
          }
          else {
                  time[i] = atof(buf);                      //here we build the array of time values
          }
          skipLine(fp);
          valid = true;
        }
        //fscanf(fp, "%s%u", rfidName, &sigStrength)  pretty sure I don't need this here...
}

void RFIDHeader::Print() {
        if (!valid)
                printf("Line not valid \n");
        else
                printf("%lf:%24s\n", time, rfidName);  //what does the %lf: do? is %lf for doubles?
}

RFIDreads readTagsAtLine(FILE* fp, int line) {
        struct RFIDreads data;
        double junkTime;
        
        unsigned long position;
        
        fseek(fp, 0, SEEK_SET);
        
        for (int i=0; i<line; i++)
               skipLine(fp);
        
        //We've gotten to the right line and are ready to read it
        
        fscanf(fp, "%lf%u", &junkTime, &data.numRFID);
        
        data.tagName = new char*[data.numRFID];
        data.sigStrength = new int[data.numRFID];
        //TODO: Free these when you're done with them by using free(data.tagName, data.sigStrength)
        
        for (int i=0; i<data.numRFID; i++) {
                fscanf(fp, "%24s%d", &data.tagName[i], &data.sigStrength[i]);  //should tagName be string?
        }
        
        return data;      // what happens to data after this function finishes?
}

unsigned int countLines(FILE *fp) {
        fseek(fp, 0, SEEK_SET);
        unsigned int lines = 0;
        while (!feof(fp)) {
                skipLine(fp);
                lines++;
        }
        fseek(fp, 0, SEEK_SET);
        return lines;
}
        
int returnLine(double time) {
        int diff[header.lines];
        for (int i=0; i < header.lines; i++)
                diff = header.time - time
        
        
        
}       
        
int main(int argc, char** argv) {
        char* filein;
        char buf[BUFSIZE];
        FILE* fin;
        char rfidName[BUFSIZE];
        int sigStrength;
        struct RFIDHeader header;
        
        fin = fopen(argv[1], "r");
        header.lines = countLines(fin);
        header.readHeader(fp);
        //TODO: Don't forget to clean up time array in the header (double* time)
        if (fin == NULL) {
                fprintf(stderr, "ERROR opening input file %s\n", argv[1]);
                return 1;
        }
        
        
        
        
        
//        while (!feof(fin)) {   //what is feof?
//                header.readHeader(fin);
//                 if (header.valid) {
//                        if (strcmp(header.rfidName, )
        
        
