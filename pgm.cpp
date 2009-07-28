/*Author: Chris Tralie
 *Project: Duke REU Fellowship 2009: Robotic navigation with RFID Waypoints
 *Purpose: To create a class that can read in binary data from a simple PGM
 *grayscale image and store the intensity values (0-255) in an array of bytes
 *where it can be easily manipulated.  This is primarily used for the occupancy
 *grids taken from pmaptest*/

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

/* PGM Image format:
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

//NOTE: I DO NOT deal with comments in the PGM image file, so it will
//be completely screwed up if a (###comment like this) is there.  GIMP
//does this, so when I was testing this and saved images from GIMP, I had
//to go into a hex editor and remove the comments
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
	//Determine the file size
	size = pgmfile.tellg();
	pgmfile.seekg(0, ios::beg);
	filedata = new BYTE[size];
	
	pgmfile.read((char*)filedata, size);
	pgmfile.close();
	parseData(filedata, size);
	delete[] filedata;
}

//The constructor for creating a new (blank) PGM File
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
		delete[] imgdata;//Clean up dynamic memory*/
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

//2D Laplacian 3x3 edge filter (in pixel space, so it's slow)
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

//Gaussian blurring (in pixel space, so it's slow)
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


//Quantize a generated map image to black and white so that the AMCL driver can
//understand it better
void PGMImage::autoQuantize() {
	int histogram[256];
	memset(histogram, 0, 256 * sizeof(int));
	double avg;
	int total = 0;
	int max = 0, maxlevel = 0;
	//Make a histogram and determine the most frequency occurring
	//pixel value.  This is the background color
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			histogram[getPixel(i, j)]++;
		}
	}
	for (int i = 0; i < 256; i++) {
		if (histogram[i] > max) {
			maxlevel = i;
			max = histogram[i];
		}
	}
	//Now take an average of the open pixels, discounting the background
	//color
    for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
                    int pixel = getPixel(i, j);
                    if (pixel != maxlevel) {
                            avg += pixel; total++;
                    }
            }
    }
    avg /= (double)total;
    cout << avg << endl;
    //Everything above the calculated average gets mapped to white,
    //everything below the average is left as it is
    for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
                    int pixel = getPixel(i, j);
                    if (pixel > avg) {
                    	setPixel(i, j, 255);
                    }
            }
    }

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


