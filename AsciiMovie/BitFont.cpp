#include "BitFont.h"

BitFont::BitFont(){
	characters = NULL;
	length = -1;
}

BitFont::~BitFont(){
	if (characters != NULL)
		delete[] characters;
}

void BitFont::printChar(int index)
{
	if (index< 0 || index>length - 1)
		return;

	for (int y = 0; y < BitFontHeight; y++, std::cout << std::endl)
		for (int x = 0; x < BitFontWidth; x++)
			std::cout << (characters[index].bitmap[y][x] < 128);
}

void BitFont::readFont(const char *fname)
{
	unsigned char rows[BitFontHeight][BitFontFileLength*BitFontWidth];

	FILE *f;
	fopen_s(&f, fname, "rb");

	if (f == NULL)
		return;

	fseek(f, 54 + 256 * 4, SEEK_SET);
	for (int i = 0; i < BitFontHeight; i++)
		fread((char*)&rows[BitFontHeight - 1 - i][0], BitFontFileLength, BitFontWidth, f);
	fclose(f);

	length = BitFontLength;
	characters = new BitCharacter[length];

	for (int i = 0; i < length; i++){
		unsigned int intensity = 0;

		for (int y = 0; y < BitFontHeight; y++)
			for (int x = 0; x < BitFontWidth; x++)
				if (i < BitFontFileLength)
					intensity += (characters[i].bitmap[y][x] = rows[y][(i%BitFontFileLength)*BitFontWidth + x]) * BitFontBackgroundRatio;
				else
					intensity += (characters[i].bitmap[y][x] = (255 - rows[y][(i%BitFontFileLength)*BitFontWidth + x])) * (2.0 - BitFontBackgroundRatio);

		intensity /= BitFontHeight*BitFontWidth;
		characters[i].intensity = intensity;

		double ccf = 0;
		
		for (int j = 0; j < BitFontWidth; j++)
			characters[i].hHisto[j] = 0.0;
		for (int j = 0; j < BitFontHeight; j++)
			characters[i].vHisto[j] = 0.0;

		for (int y = 0; y < BitFontHeight; y++){
			for (int x = 0; x < BitFontWidth; x++){
				characters[i].delta[y][x] = (int)(characters[i].bitmap[y][x]) - intensity;
				ccf += characters[i].delta[y][x]*characters[i].delta[y][x];
				characters[i].hHisto[x] += characters[i].bitmap[y][x];
				characters[i].vHisto[y] += characters[i].bitmap[y][x];
			}
		}

		for (int j = 0; j < BitFontWidth; j++)
			characters[i].hHisto[j] /= 255.0 * BitFontHeight;
		for (int j = 0; j < BitFontHeight; j++)
			characters[i].vHisto[j] /= 255.0 * BitFontWidth;

		characters[i].nccf = sqrt(ccf);
		//std::cout << (char)((i%BitFontFileLength) + 32) << " " << characters[i].intensity << " " << characters[i].nccf << std::endl;
	}
	std::cout << length << " characters are loaded." << std::endl;
	//system("pause");
}

void BitFont::normalize(bool streatch){
	if (streatch) {
		//histogram streaching
		int min = 256, max = 0;

		for (int i = 0; i<length; i++){
			if (characters[i].intensity < min)
				min = characters[i].intensity;

			if (characters[i].intensity > max)
				max = characters[i].intensity;
		}

		for (int i = 0; i<length; i++){
			characters[i].intensity = (255.0 *(float)(characters[i].intensity - min) / (float)(max - min));
		}
	}
	else{
		//histogram equilization
		int pmf[256] = {};
		for (int i = 0; i < length; i++)
			pmf[characters[i].intensity]++;

		int cdf[256] = {}, intensityMapping[256] = {};
		cdf[0] = pmf[0];
		for (int i = 1; i < length; i++)
			cdf[i] = cdf[i - 1] + pmf[i];

		float ideadBinSize = (BitFontWidth*BitFontHeight) / 255.0;
		for (int i = 0, j = 0; i < 256;){
			int icdf = ideadBinSize*(j + 1); //ideal CDF
			if (j >= 255 || icdf >= cdf[i])
				intensityMapping[i++] = j;
			else
				j++;
		}

		for (int i = 0; i < length; i++){
			characters[i].intensity = intensityMapping[characters[i].intensity];
		}
	}
}