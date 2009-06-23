//g++ -o rfid rfid.cpp
//input should be ./rfid [name of log file] [time to find]



//This file converts the output log file of the RFID into a format which
//SLAM will be able to interpret


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 256

void skipLine(FILE* fp) {
        char c;
        do {
                c = fgetc(fp);
        }
        while ((c >= 0) && (c != '\n'));
}

struct RFIDHeader {
        bool valid;
        unsigned int lines;
        double * time;
        
        void readHeader(FILE* fp);
        void Print();
};

struct RFIDreads {
        char** tagName;
        int * sigStrength;
        int numRFID;
};

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
        
int main(int argc, char** argv) {
        //input should be ./rfid [name of log file] [time to find]
        printf("Main\n");
        char* filein;
        char buf[BUFSIZE];
        FILE* fin;
        int sigStrength;
        struct RFIDHeader header;
        printf("Initialized\n");
        
        fin = fopen(argv[1], "r");
        //TODO: Don't forget to clean up time array in the header (double* time)
        if (fin == NULL) {
                fprintf(stderr, "ERROR opening input file %s\n", argv[1]);
                return 1;
        }
        
        header.lines = countLines(fin);
        header.readHeader(fin);
        for (int i=0; i<header.lines; i++)
                printf("%lf\n", header.time[i]);
                
        printf("time to find: %lf\n", atof(argv[2]));
        struct RFIDreads data = readTagsAtLine(fin, returnLine(atof(argv[2]), header));
        
        for (int i=0; i<data.numRFID; i++) {
                printf("%s %d\n", data.tagName[i], data.sigStrength[i]);
                free(data.tagName[i]);
        }
        
        free(data.tagName);
        free(data.sigStrength);
        free(header.time);
}

        
        
