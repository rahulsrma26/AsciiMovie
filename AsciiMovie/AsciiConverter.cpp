#include "AsciiConverter.h"

int intensityMapping[256];
void createIntensityMapping(BitFont& font, int normalise)
{
	if (normalise == 1) //histogram streaching
	{
		int min = 256, max = 0;

		for (int i = 0; i<BitFontLength; i++)
		{
			if (font.characters[i].intensity < min)
				min = font.characters[i].intensity;

			if (font.characters[i].intensity > max)
				max = font.characters[i].intensity;
		}

		for (int i = 0; i<BitFontLength; i++)
		{
			font.characters[i].intensity = (255.0 *(float)(font.characters[i].intensity - min) / (float)(max - min));
			//cout<< (char)(i+32)<< " "<< font.characters[i].intensity<<endl;
		}
	}
	else if (normalise == 2) //histogram equilization
	{
		int pmf[256] = {};
		for (int i = 0; i < BitFontLength; i++)
			pmf[font.characters[i].intensity]++;

		int cdf[256] = {};
		cdf[0] = pmf[0];
		for (int i = 1; i < BitFontLength; i++)
			cdf[i] = cdf[i - 1] + pmf[i];

		float ideadBinSize = (BitFontWidth*BitFontHeight) / 255.0;
		for (int i = 0, j = 0; i < 256; )
		{
			int icdf = ideadBinSize*(j+1); //ideal CDF
			if (j >= 255 || icdf >= cdf[i])
				intensityMapping[i++] = j;
			else
				j++;
		}

		for (int i = 0; i < BitFontLength; i++){
			font.characters[i].intensity = intensityMapping[font.characters[i].intensity];
			std::cout << (char)(i + 32) << " - " << font.characters[i].intensity << std::endl;
		}
	}

	for (int i = 0; i<256; i++)
	{
		int min = 255, minIdx = 0;
		for (int j = 0; j<BitFontLength; j++)
			if (abs(font.characters[j].intensity - i) < min)
			{
			min = abs(font.characters[j].intensity - i);
			minIdx = j;
			}

		intensityMapping[i] = minIdx;
		//cout<< (char)(intensityMapping[i])<< " "<< i<<endl;
	}
}

#define HD 0
#define VD 0

int* getCrossCorrASCII(BitFont& font, Mat& frame, int threshold, int* lastText)
{
	unsigned char* img = (unsigned char*)frame.data;
	int imgw = frame.cols;
	int imgh = frame.rows;

	int idx = 0;
	int blockR = (imgh - 2*VD) / BitFontHeight;
	int blockC = (imgw - 2*HD) / BitFontWidth;
	int* text = new int[blockC*blockR];
	int avgMatches = 0;
	char ascii[] = " !\"#$%&'()*+,-/02346789;<=>?@ACEFGHIJKLMNOPRSTUWXYZ[\]^_`abcdefgijklmnopqrstuvwxyz{|}~";
	int stats[BitFontFileLength];

	for (int y = 0; y<blockR; y++)
	{
		for (int x = 0; x<blockC; x++)
		{
			int intensity = 0;
			for (int i = 0; i<BitFontHeight - 2*VD; i++)
				for (int j = 0; j<BitFontWidth - 2*HD; j++)
					intensity += img[(y*BitFontHeight + i + VD)*imgw + x*BitFontWidth + j + HD];
			intensity /= (BitFontHeight - 2*VD)*(BitFontWidth - 2*HD);

			if (lastText != NULL && abs(intensity - font.characters[lastText[idx]].intensity) < threshold/2){
				text[idx] = lastText[idx];
				idx++;
				continue;
			}

			double ncc[2 * VD + 1][2 * HD + 1];

			for (int dy = 0; dy < 2*VD+1; dy++)
			{
				for (int dx = 0; dx < 2*HD+1; dx++)
				{
					int imgy = y*BitFontHeight + dy;
					int imgx = x*BitFontWidth + dx;
					
					ncc[dy][dx] = 0.0;
					for (int i = 0; i < BitFontHeight; i++)
						for (int j = 0; j < BitFontWidth; j++)
							ncc[dy][dx] += (img[(imgy + i)*imgw + imgx + j] - intensity)*(img[(imgy + i)*imgw + imgx + j] - intensity);
					ncc[dy][dx] = sqrt(ncc[dy][dx]);
				}
			}

			double max = -9999999999999.0;
			int maxi = 0;
			int nom = 0;
			for (int k = 0; k<BitFontLength; k++) //character index
			{
				if (abs(intensity - font.characters[k].intensity) > threshold)
					continue;

				for (int dy = 0; dy < 2 * VD + 1; dy++)
				{
					for (int dx = 0; dx < 2 * HD + 1; dx++)
					{
						int imgy = y*BitFontHeight + dy;
						int imgx = x*BitFontWidth + dx;

						double ncci = 0.0;

						for (int i = 0; i < BitFontHeight; i++)
							for (int j = 0; j < BitFontWidth; j++)
								ncci += (img[(imgy + i)*imgw + imgx + j] - intensity)*(font.characters[k].bitmap[i][j] - font.characters[k].intensity);

						double cc = ncci/((ncc[dy][dx]+0.01)*(font.characters[k].nccf+0.01));
						//double cc = ncci;

						if (cc > max)
						{
							max = cc;
							maxi = k;
						}
					}
				}

				nom++;
			}

			text[idx++] = maxi;
			avgMatches += nom;
			//std::cout<< maxi<< ","<< nom<< "\n";
		}
	}

	std::cout << "Average Matches "<< avgMatches/(blockR*blockC)<< "\n";
	return text;
}

void genFrameFromText(BitFont& font, Mat& frame, int* text){
	unsigned char* pixelPtr = (unsigned char*)frame.data;
	int w = (frame.cols - 2*HD) / BitFontWidth;
	int h = (frame.rows - 2*VD) / BitFontHeight;
	int idx = 0;

	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			for (int i = 0; i < BitFontHeight; i++)
				for (int j = 0; j < BitFontWidth; j++)
					pixelPtr[(y*BitFontHeight + i)*frame.cols + (x*BitFontWidth + j) + 0] = font.characters[text[idx]].bitmap[i][j];
			idx++;
		}
	}
}

inline int getStreatchedColor(int c, int s)
{
	int n = (c - 127)*s + 127;
	return (n < 0) ? 0 : (n>255 ? 255 : n);
}

void genColorFrameFromText(BitFont& font, Mat& colorFrame, int* text){
	unsigned char* colorPtr = (unsigned char*)colorFrame.data;
	int ch = colorFrame.channels();
	int imgw = colorFrame.cols;
	int imgh = colorFrame.rows;
	int w = (colorFrame.cols - 2*HD) / BitFontWidth;
	int h = (colorFrame.rows - 2*VD) / BitFontHeight;
	int idx = 0;

	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			int avg[3] = {};
			for (int i = 0; i < BitFontHeight; i++)
				for (int j = 0; j < BitFontWidth; j++)
					for (int k = 0; k < 3; k++){
						int c = getStreatchedColor(colorPtr[((y*BitFontHeight + i)*imgw + (x*BitFontWidth + j))*ch + k], 2) + 128;
						avg[k] += (c>255) ? 255 : c;
					}

			for (int k = 0; k < 3; k++)
				avg[k] /= (BitFontHeight*BitFontWidth);

			avg[0] = (((avg[0] - 128) >> 5) << 5) + 128;
			avg[1] = (((avg[1] - 128) >> 5) << 5) + 128;
			avg[2] = (((avg[2] - 128) >> 6) << 6) + 128;

			for (int i = 0; i < BitFontHeight; i++)
				for (int j = 0; j < BitFontWidth; j++)
					for (int k = 0; k < 3; k++)
						colorPtr[((y*BitFontHeight + i)*imgw + (x*BitFontWidth + j))*ch + k] = (font.characters[text[idx]].bitmap[i][j] * avg[k]) >> 8;

			idx++;
		}
	}
}

int* genASCII(BitFont& font, Mat& frame){
	unsigned char* pixelPtr = (unsigned char*)frame.data;
	int w = (frame.cols-2) / BitFontWidth;
	int h = (frame.rows-2) / BitFontHeight;
	int* text = new int[w*h];
	int idx = 0;

	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			int it = 0;
			for (int i = 0; i < BitFontHeight; i++)
				for (int j = 0; j < BitFontWidth; j++)
				{
				it += pixelPtr[(y*BitFontHeight + i)*frame.cols + (x*BitFontWidth + j)];
				/*it += ( pixelPtr[(y*BitFontHeight + i)*frame.cols + (x*BitFontWidth + j) + 0]
				+ pixelPtr[(y*BitFontHeight + i)*frame.cols + (x*BitFontWidth + j) + 1]
				+ pixelPtr[(y*BitFontHeight + i)*frame.cols + (x*BitFontWidth + j) + 2]) / 3;*/
				}

			it /= BitFontHeight*BitFontWidth;
			text[idx++] = intensityMapping[it];
		}
	}

	return text;
}