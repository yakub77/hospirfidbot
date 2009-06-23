struct RFIDHeader {
        bool valid;
        unsigned int lines;
        double * time;
        
        void readHeader(FILE* fp);
};

struct RFIDreads {
        char** tagName;
        int * sigStrength;
        int numRFID;
};

void skipLine(FILE*);
void RFIDHeader::readHeader(FILE* fp);
RFIDreads readTagsAtLine(FILE* fp, int line);
unsigned int countLines(FILE *fp);
int returnLine(double time, RFIDHeader header);
int main(int argc, char** argv);



