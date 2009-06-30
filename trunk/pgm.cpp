#include "pgm.h"

//Use "bless" hex editor to check my work

bool isWhiteSpace(string* sdata, int index) {
	const char* pointer = sdata->data();
	if (pointer[index] == '\r' || pointer[index] == '\t' || 
		pointer[index] == '\n' || pointer[index] == ' ')
		return true;
	return false;
}

void skipWhiteSpace(string* sdata, ptype* index) {
	while (isWhiteSpace(sdata, (*index)) && (*index) < sdata->size())
		(*index) = (*index) + 1;
}

int getint(string* sdata, ptype* index) {
	ptype start = *index;
	while (!isWhiteSpace(sdata, (*index)) && (*index) < sdata->size())
		(*index) = (*index) + 1;
	string intarray = sdata->substr(start, (*index - start + 1));
	return atoi(intarray.data());
}

/*
- A "magic number" for identifying the  file  type.   A  pgm
       file's magic number is the two characters "P2".
     - Whitespace (blanks, TABs, CRs, LFs).
     - A width, formatted as ASCII characters in decimal.
     - Whitespace.
     - A height, again in ASCII decimal.
     - Whitespace.
     - The maximum gray value, again in ASCII decimal.
     - Whitespace.
     - Width * height gray values, each in ASCII decimal, between
       0  and  the  specified  maximum  value,  separated by whi-
       tespace, starting at the top-left corner of  the  graymap,
       proceeding  in normal English reading order.  A value of 0
       means black, and the maximum value means white.

*/
void PGMImage::parseData(BYTE* data, ptype size) {
	string sdata((char*)data);
	//Optional: Output magic number P5
	ptype index = 2;
	skipWhiteSpace(&sdata, &index);
	width = getint(&sdata, &index);
	skipWhiteSpace(&sdata, &index);
	height = getint(&sdata, &index);
	skipWhiteSpace(&sdata, &index);
	maxvalue = getint(&sdata, &index);
	skipWhiteSpace(&sdata, &index);
	////////////////////////////////////////
	///Now store the image data
	///////////////////////////////////////
	skipWhiteSpace(&sdata, &index);
	int imgindex = 0;
	cout << width << " x " << height << "\n\n\n";
	imgdata = new BYTE[width * height];
	while (index < size && imgindex < width * height) {
		imgdata[imgindex] = data[index];
		index++;imgindex++;
	}
}

void PGMImage::writeToFile(char* filename) {
	ofstream pgmfile;
	pgmfile.open(filename, ios::binary | ios::out);
	pgmfile << "P5 " << width << " " << height << " 255 ";
	pgmfile.write((char*)imgdata, width * height);
}


//The constructor for opening a PGM image from a file
PGMImage::PGMImage(char* filename) {
	BYTE* filedata;
	ifstream pgmfile;
	ptype size;
	pgmfile.open(filename, ios::binary | ios::in | ios::ate);
	if (!pgmfile.is_open()) {
		cerr << "ERROR: File failed to open file " << filename << endl;
		return;
	}
	size = pgmfile.tellg();
	pgmfile.seekg(0, ios::beg);
	filedata = new BYTE[size];
	
	pgmfile.read((char*)filedata, size);
	pgmfile.close();
	parseData(filedata, size);
	free(filedata);
}

//The constructor for creating a new PGM File
PGMImage::PGMImage(int w, int h) {
	width = w;
	height = h;
	imgdata = new BYTE[w * h];
	for (int i = 0; i < w; i++) {
		for (int j = 0; j < h; j++) {
			setPixel(i, j, 0);
		}
	}
}


PGMImage::~PGMImage() {
	if (imgdata != NULL)
		free(imgdata);//Clean up dynamic memory*/
}

//i is position along the width, j is position along the height
//(0, 0) is upper left of image
BYTE PGMImage::getPixel(int i, int j) {
	assert (i < width && j < width);
	return imgdata[j * width + i];
}

void PGMImage::setPixel(int i, int j, BYTE value) {
	assert (i < width && j < width);
	imgdata[j * width + i] = value;
}


//Test client for this class
int main(int argc, char** argv) {
	PGMImage test("coarse.pgm");
	for (int i = 0; i < test.height; i++) {
		test.setPixel(3, i, 255);
		test.setPixel(19, i, 255);
	}
	test.writeToFile("out.pgm");
	return 0;
}


