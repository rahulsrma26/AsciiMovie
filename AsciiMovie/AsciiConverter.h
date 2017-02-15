#ifndef ASCIICONVERTER_H

#define ASCIICONVERTER_H

#include <opencv2/highgui/highgui.hpp>
#include "BitFont.h"

using namespace cv;

typedef struct {
	unsigned char *text;
	unsigned char *color;
	int w;
	int h;
}AsciiText;

void createIntensityMapping(BitFont& font, int normalise);

int* getCrossCorrASCII(BitFont& font, Mat& frame, int threshold, int* lastText = NULL);

void genFrameFromText(BitFont& font, Mat& frame, int* text);
void genColorFrameFromText(BitFont& font, Mat& colorFrame, int* text);

int* genASCII(BitFont& font, Mat& frame);

#endif