#ifndef BITFONT_H

#define BITFONT_H

#include <iostream>
#include <cstdio>
#include <cmath>

#define BitFontWidth (8)
#define BitFontHeight (12)
#define BitFontBackgroundRatio (1.0)
#define BitFontFileLength (180)
#define BitFontLength (BitFontFileLength*2)

typedef struct {
	unsigned char bitmap[BitFontHeight][BitFontWidth];
	int delta[BitFontHeight][BitFontWidth];
	int intensity;
	double nccf; // normalised cross-correlation factor
	float hHisto[BitFontWidth];
	float vHisto[BitFontHeight];
}BitCharacter;

class BitFont{
public:
	BitCharacter *characters;
	int length;	// number of characters

	BitFont();
	~BitFont();

	void printChar(int index);
	void readFont(const char *fname);
	void normalize(bool streatch = true);
};


#endif