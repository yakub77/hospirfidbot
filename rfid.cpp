//g++ -o rfid rfid.cpp
//input should be ./rfid [name of log file] [time to find]



//This file converts the output log file of the RFID into a format which
//SLAM will be able to interpret



#include "rfid.h"

#define BUFSIZE 256

struct RFIDHeader {
        bool valid;
        unsigned int lines;
        double * time;
        
        void readHeader(FILE* fp);
};

void skipLine(FILE* fp) {
        char c;
        do {
                c = fgetc(fp);
        }
        while ((c >= 0) && (c != '\n'));
}

void RFIDHeader::readHeader(FILE* fp) {
        char buf[BUFSIZE];
        time = new double[lines];
        
        for(int i=0; i<lines; i++) {
                fscanf(fp, "%20s", buf);
                if (buf[0] == '#') {
                        // A comment is encountered, assume this is not a valid header
                        // Skip everything in a line that's started with a comment
                        valid = false;
                        return;
                }
                else {
                        time[i] = atof(buf);    //here we build the array of time values
                }
                skipLine(fp);
                valid = true;
        }
        //fscanf(fp, "%s%u", rfidName, &sigStrength)  pretty sure I don't need this here...
}

void RFIDreads::freeRead() {
        for (int i=0; i<numRFID; i++) {
                free(tagName[i]);
        }
        free(tagName);
        free(sigStrength);
}

RFIDreads readTagsAtLine(FILE* fp, int line) {
        struct RFIDreads data;
        double time;
        fpos_t file_loc;
        
        fseek(fp, 0, SEEK_SET);
        for (int i=0; i<line; i++)
               skipLine(fp);
        fgetpos(fp, &file_loc);
        
        //We've gotten to the right line and are ready to read it
        fscanf(fp, "%lf%u", &time, &data.numRFID);
        printf("Reading at time %lf\n", time);
        printf("numRFID: %u\n", data.numRFID);
        
        data.tagName = new char*[data.numRFID];
        data.sigStrength = new int[data.numRFID];
        //TODO: Free these when you're done with them by using free(data.tagName, data.sigStrength)
        
        for (int i=0; i<data.numRFID; i++) {
                data.tagName[i] = new char[24];
                fscanf(fp, "%24s%d", data.tagName[i], &data.sigStrength[i]);  //should tagName be string?
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
        
int returnLine(double time, RFIDHeader header) {
        printf("time = %lf\n", time);
        double diff[header.lines];
        double leastdiff = time;
        int position = 0;
        
        
        for (int i=0; i<header.lines; i++) {
                diff[i] = header.time[i] - time;
                if (diff[i]<0)   
                        diff[i] = -diff[i];
                if (diff[i]<leastdiff) {
                        leastdiff = diff[i]; 
                        position = i;
                }
        }
        printf("position = %u\n", position);
        return position;
}       
        
RFIDreads getRFIDreads(char* filename, float time) {
        //input should be ./rfid [name of log file] [time to find]
        char* filein;
        char buf[BUFSIZE];
        FILE* fin;
        int sigStrength;
        struct RFIDHeader header;
        struct RFIDreads data;
        
        fin = fopen(filename, "r");
        if (fin == NULL) {
                fprintf(stderr, "ERROR opening input file %s\n", filename);
                return data;
        }
        
        header.lines = countLines(fin);
        header.readHeader(fin);
        /*for (int i=0; i<header.lines; i++)
                printf("%lf\n", header.time[i]);
                
        printf("time to find: %lf\n", atof(argv[2]));*/
        data = readTagsAtLine(fin, returnLine(time, header));
        free(header.time);
        return data;
}

        
        
