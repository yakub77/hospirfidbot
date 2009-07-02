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
	//cout << width << " x " << height << "\n\n\n";
	imgdata = new BYTE[width * height];
	while (index < size && imgindex < width * height) {
		imgdata[imgindex] = data[index];
		index++;imgindex++;
	}
}

void PGMImage::writeToFile(const char* filename) {
	ofstream pgmfile;
	pgmfile.open(filename, ios::binary | ios::out);
	pgmfile << "P5 " << width << " " << height << " " << maxvalue <<" ";
	pgmfile.write((char*)imgdata, width * height);
}


PGMImage::PGMImage() {}

//The constructor for opening a PGM image from a file
PGMImage::PGMImage(const char* filename) {
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
	maxvalue = 255;
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

//Quantize into an entirely black and white image
void PGMImage::Quantize(BYTE cutoff) {
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			if (getPixel(i, j) < cutoff) setPixel(i, j, 0);
			else setPixel(i, j, 255);
		}
	}
}


PGMImage PGMImage::getEdges() {
	PGMImage toReturn(width, height);
	for (int i = 1; i < width - 1; i++) {
		for (int j = 1; j < height - 1; j++) {
			int total = 8 * getPixel(i, j);
			for (int p = -1; p <= 1; p++) {
				for (int q = -1; q <= 1; q++) {
					if (!(p == 0 && q == 0))
						total -= getPixel(i + p, j + q);
				}
			}
			toReturn.setPixel(i, j, (BYTE)total);
		}
	}
	return toReturn;
}

//Gaussian blurring
void PGMImage::Blur(int w) {
	for (int i = w; i < width - w; i++) {
		for (int j = w; j < height - w; j++) {
			double total = 0;
			double weight = 0;
			for (int p = -w; p <= w; p++) {
				for (int q = -w; q <= w; q++) {
					double rSquared = p*p + q*q;
					double expon = pow (2.0, -rSquared);
					weight += expon;
					total += expon * getPixel(i + p, j + q);
				}
			}
			total /= weight;
			setPixel(i, j, (BYTE)(total));
		}
	}
}

void PGMImage::autoQuantize() {
	double avg;
	int min;
	int total = 0;
	//Find minimum pixel value and discount it from average
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			int pixel = getPixel(i, j);
			if (pixel < min) min = pixel;
		}
	}
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			int pixel = getPixel(i, j);
			if (pixel > min) {
				avg += pixel; total++;
			}
		}
	}
	avg /= (double)total;
	cout << avg << endl;
	Quantize((int)avg);
}

//Test client for this class
/*int main(int argc, char** argv) {
	PGMImage map("logs/tags.pgm");
	printf("%i x %i\n", map.width, map.height);
	for (int i = 0; i < map.width; i++) {
		for (int j = 0; j < map.height; j++) {
			if (map.getPixel(i, j) < 255)
				printf("(%i, %i)\n", i, j);
		}
	}
	return 0;
}*/


