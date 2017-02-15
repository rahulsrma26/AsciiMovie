#ifndef AMPASCII_H

#define AMPASCII_H

#include <opencv2/highgui/highgui.hpp>
#include <amp.h>
#include <cstdint>
#include "BitFont.h"

using namespace cv;

typedef struct {
	int w;
	int h;
	int *text;
	unsigned char *color;
}TextBlock;

typedef struct {
	int length;

	int* delta;
	int* intensity;
	float* rsd2;
}AmpFontStream;

typedef struct {
	int w;
	int h;

	int* delta;
	int* intensity;
	float* rsd2;
}AmpImageStream;

typedef struct {
	int w;
	int h;

	float* hHisto;
	float* vHisto;
}AmpImageStream2;

AmpFontStream* getAmpFontStream(const BitFont& font);
TextBlock* imageToAscii(const AmpFontStream& font, const Mat& frame, int threshold, Mat* color = NULL);
TextBlock* imageToAscii2(const BitFont& font, const Mat& frame, int threshold, Mat* color = NULL);
void genFrameFromText(Mat& frame, const BitFont& font, const TextBlock& text, bool color = false);

#endif