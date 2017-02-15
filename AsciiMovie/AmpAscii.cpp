#include "AmpAscii.h"

AmpFontStream* getAmpFontStream(const BitFont& font){
	AmpFontStream *data = new AmpFontStream{ font.length, new int[font.length*BitFontHeight*BitFontWidth], new int[font.length], new float[font.length] };

	for (int f = 0, idx = 0; f < font.length; f++){
		for (int r = 0, i = 0; r < BitFontHeight; r++){
			for (int c = 0; c < BitFontWidth; c++, i++, idx++){
				data->delta[idx] = font.characters[f].delta[r][c];
			}
		}
		data->rsd2[f] = font.characters[f].nccf;
		data->intensity[f] = font.characters[f].intensity;
	}

	return data;
}

AmpImageStream* getAmpImageStream(const Mat& frame){	// root sum of delta square
	unsigned char* img = (unsigned char*)frame.data;
	int imgw = frame.cols;
	int imgh = frame.rows;
	int brc = imgh / BitFontHeight;
	int bcc = imgw / BitFontWidth;

	AmpImageStream *data = new AmpImageStream{ imgw, imgh, new int[imgh*imgw], new int[brc*bcc], new float[brc*bcc] };

	for (int y = 0, idxSD = 0; y < brc; y++){
		for (int x = 0; x < bcc; x++, idxSD++){
			int yH = y*BitFontHeight, xW = x*BitFontWidth;
			
			int mean = 0;
			for (int i = 0; i < BitFontHeight; i++){
				for (int j = 0; j < BitFontWidth; j++){
					mean += img[(yH + i)*imgw + xW + j];
				}
			}
			mean /= (BitFontHeight*BitFontWidth);
			data->intensity[idxSD] = mean;

			float sd2 = 0;
			for (int i = 0; i < BitFontHeight; i++){
				for (int j = 0; j < BitFontWidth; j++){
					int idx = (yH + i)*imgw + xW + j;
					data->delta[idx] = img[idx] - mean;
					sd2 += data->delta[idx] * data->delta[idx];
				}
			}
			data->rsd2[idxSD] = sqrt(sd2);
		}
	}

	return data;
}

AmpImageStream2* getAmpImageStream2(const Mat& frame) {	// root sum of delta square
	unsigned char* img = (unsigned char*)frame.data;
	int imgw = frame.cols;
	int imgh = frame.rows;
	int brc = imgh / BitFontHeight;
	int bcc = imgw / BitFontWidth;

	AmpImageStream2 *data = new AmpImageStream2{ imgw, imgh, new float[brc*bcc*BitFontWidth], new float[brc*bcc*BitFontHeight] };

	for (int y = 0, idxSD = 0; y < brc; y++) {
		for (int x = 0; x < bcc; x++, idxSD++) {
			int yH = y*BitFontHeight, xW = x*BitFontWidth;
			int hHistoIdx = idxSD * BitFontWidth;
			int vHistoIdx = idxSD * BitFontHeight;
			
			for (int i = 0; i < BitFontWidth; i++)
				data->hHisto[hHistoIdx + i] = 0.0;
			for (int i = 0; i < BitFontHeight; i++)
				data->vHisto[vHistoIdx + i] = 0.0;

			for (int i = 0; i < BitFontHeight; i++) {
				for (int j = 0; j < BitFontWidth; j++) {
					data->hHisto[hHistoIdx + j] += img[(yH + i)*imgw + xW + j];
					data->vHisto[vHistoIdx + i] += img[(yH + i)*imgw + xW + j];
				}
			}

			for (int i = 0; i < BitFontWidth; i++)
				data->hHisto[hHistoIdx + i] /= 255.0 * BitFontHeight;
			for (int i = 0; i < BitFontHeight; i++)
				data->vHisto[vHistoIdx + i] /= 255.0 * BitFontWidth;
		}
	}

	return data;
}

float* getNccScore(const AmpFontStream& font, const AmpImageStream& img, int threshold){
	int brc = img.h / BitFontHeight;
	int bcc = img.w / BitFontWidth;
	float* scores = new float[font.length*brc*bcc];

	for (int y = 0, idxSD = 0, idxFont = 0; y < brc; y++){
		for (int x = 0; x < bcc; x++, idxSD++){
			int yH = y*BitFontHeight, xW = x*BitFontWidth;
			for (int f = 0; f < font.length; f++, idxFont++){
				
				if (abs(img.intensity[idxSD] - font.intensity[f]) > threshold){
					scores[idxFont] = -9999999999999.9;
					continue;
				}

				scores[idxFont] = -9999999999999.0;
				for (int itr = 0; itr < 9; itr++){
					int dx = (itr % 3) - 1;
					int dy = (itr / 3) - 1;
					if ((x == 0 && dx == -1) || (x == bcc - 1 && dx == 1) || (y == 0 && dy == -1) || (y == brc - 1 && dy == 1))
						continue;

					float ncci = 0;
					for (int i = 0, k = 0; i < BitFontHeight; i++){
						for (int j = 0; j < BitFontWidth; j++, k++){
							ncci += img.delta[(yH + i + dy)*img.w + xW + j + dx] * font.delta[f*BitFontHeight*BitFontWidth + k];
						}
					}
					ncci /= (img.rsd2[idxSD] + 0.01) * (font.rsd2[f] + 0.01);
					if (ncci > scores[idxFont])
						scores[idxFont] = ncci;
				}
			}
		}
	}

	return scores;
}

float* getNccScoreAmp(const AmpFontStream& font, const AmpImageStream& img, int threshold){
	int brc = img.h / BitFontHeight;
	int bcc = img.w / BitFontWidth;

	concurrency::array_view<const int, 3> fontDelta(font.length, BitFontHeight, BitFontWidth, font.delta);
	concurrency::array_view<const int, 1> fontMean(font.length, font.intensity);
	concurrency::array_view<const float, 1> fontRsd2(font.length, font.rsd2);

	concurrency::array_view<const int, 2> imgDelta(img.h, img.w, img.delta);
	concurrency::array_view<const int, 2> imgMean(brc, bcc, img.intensity);
	concurrency::array_view<const float, 2> imgRsd2(brc, bcc, img.rsd2);

	float* scores = new float[font.length*brc*bcc];
	concurrency::array_view<float, 3> scoresAmp(brc, bcc, font.length, scores);
	scoresAmp.discard_data();

	concurrency::parallel_for_each(
		scoresAmp.extent,
		[=](concurrency::index<3> idx) restrict(amp)
	{
		scoresAmp[idx] = -9999999999999.9;
		float delta = imgMean(idx[0], idx[1]) - fontMean(idx[2]);
		if (delta < 0)
			delta = -delta;
		
		float intWeight = imgMean(idx[0], idx[1]) - 128.0;
		if (intWeight < 0)
			intWeight = -intWeight;
		intWeight += 32.0;

		if (delta < threshold){
			float prod = (imgRsd2(idx[0], idx[1]) + 0.001)*(fontRsd2(idx[2]) + 0.001);
			for (int itr = 0; itr < 9; itr++){
				int dx = (itr % 3) - 1;
				int dy = (itr / 3) - 1;
				if ((idx[1] == 0 && dx == -1) || (idx[1] == bcc - 1 && dx == 1) || (idx[0] == 0 && dy == -1) || (idx[0] == brc - 1 && dy == 1))
					continue;
				
				float ncci = 0;
				for (int i = 0; i < BitFontHeight; i++){
					for (int j = 0; j < BitFontWidth; j++){
						ncci += imgDelta(idx[0] * BitFontHeight + i + dy, idx[1] * BitFontWidth + j + dx) * fontDelta(idx[2], i, j);
					}
				}
				ncci /= prod;
				if (ncci > scoresAmp[idx])
					scoresAmp[idx] = ncci + (intWeight / 96.0) * (threshold - delta) / (float)threshold;
			}
		}
	});

	scoresAmp.synchronize();
	return scores;
}

//#define AsciiMatrix16color
#define AsciiMatrixMatrixcolor

TextBlock* imageToAscii(const AmpFontStream& font, const Mat& frame, int threshold, Mat* colorBlockFrame){
	AmpImageStream *img = getAmpImageStream(frame);
	float* scores = getNccScoreAmp(font, *img, threshold);
	
	TextBlock* tb = new TextBlock{ img->w / BitFontWidth, img->h / BitFontHeight, NULL, NULL };
	tb->text = new int[tb->w*tb->h];

	for (int r = 0, i = 0, fi = 0; r < tb->h; r++){
		for (int c = 0; c < tb->w; c++, i++){
			int maxf = 0;
			float max = -9999999999999.0;
			
			for (int f = 0; f < font.length; f++, fi++){
				if (scores[fi] > max){
					max = scores[fi];
					maxf = f;
				}
			}
			
			tb->text[i] = maxf;
		}
	}

	delete[] img->delta;
	delete[] img->rsd2;
	delete img;
	delete[] scores;

	if (colorBlockFrame != NULL){
		tb->color = new unsigned char[tb->w*tb->h];
		int ch = colorBlockFrame->channels();
		unsigned char *colimg = colorBlockFrame->data;
		for (int r = 0, i = 0; r < tb->h; r++){
			for (int c = 0; c < tb->w; c++, i++){
#ifdef AsciiMatrix16color
				tb->color[i] = ((colimg[i*ch + 0] >> 7) << 3) | ((colimg[i*ch + 1] >> 6) << 1) | (colimg[i*ch + 2] >> 7);
#else
#ifdef AsciiMatrixMatrixcolor
				tb->color[i] = (colimg[i*ch + 0] * 19595 + colimg[i*ch + 1] * 38470 + colimg[i*ch + 2] * 7471) >> 22;
#else
				tb->color[i] = ((colimg[i*ch + 0] >> 5) << 5) | ((colimg[i*ch + 1] >> 5) << 2) | (colimg[i*ch + 2] >> 6);
#endif		
#endif
			}
		}
	}

	return tb;
}

constexpr auto sqr(float x) { return x*x; }

int* getNccScore2(const BitFont& font, const AmpImageStream2& img, int threshold) {
	int brc = img.h / BitFontHeight;
	int bcc = img.w / BitFontWidth;
	auto* chars = new int[brc*bcc];

	for (int y = 0, idxSD = 0, idxFont = 0; y < brc; y++) {
		for (int x = 0; x < bcc; x++, idxSD++) {
			int yH = y*BitFontHeight, xW = x*BitFontWidth;
			int hHistoIdx = idxSD * BitFontWidth;
			int vHistoIdx = idxSD * BitFontHeight;
			float highScore = -99999.99;
			size_t highChar = 0;
			for (size_t f = 0; f < font.length; f++, idxFont++) {
				double vScore = 0.0, hScore = 0.0;
				for (int i = 0; i < BitFontWidth; i++)
					hScore += sqr(img.hHisto[hHistoIdx + i] - font.characters[f].hHisto[i]);
				for (int i = 0; i < BitFontHeight; i++)
					vScore += sqr(img.vHisto[vHistoIdx + i] - font.characters[f].vHisto[i]);
				float score = 1.0 - hScore * vScore;
				if (score > highScore) {
					highScore = score;
					highChar = f;
				}
			}
			chars[idxSD] = highChar;
		}
	}

	return chars;
}

TextBlock* imageToAscii2(const BitFont& font, const Mat& frame, int threshold, Mat* colorBlockFrame) {
	AmpImageStream2 *img = getAmpImageStream2(frame);
	TextBlock* tb = new TextBlock{ img->w / BitFontWidth, img->h / BitFontHeight, NULL, NULL };
	tb->text = getNccScore2(font, *img, threshold);

	delete[] img->hHisto;
	delete[] img->vHisto;
	delete img;

	if (colorBlockFrame != NULL) {
		tb->color = new unsigned char[tb->w*tb->h];
		int ch = colorBlockFrame->channels();
		unsigned char *colimg = colorBlockFrame->data;
		for (int r = 0, i = 0; r < tb->h; r++) {
			for (int c = 0; c < tb->w; c++, i++) {
				tb->color[i] = ((colimg[i*ch + 0] >> 5) << 5) | ((colimg[i*ch + 1] >> 5) << 2) | (colimg[i*ch + 2] >> 6);
			}
		}
	}

	return tb;
}


void genFrameFromText(Mat& frame, const BitFont& font, const TextBlock& text, bool color){
	unsigned char* pixelPtr = (unsigned char*)frame.data;
	if (color){
		unsigned int imgw = frame.cols, imgh = frame.rows;
		for (unsigned int y = 0, idx = 0; y < text.h; y++){
			for (unsigned int x = 0; x < text.w; x++, idx++){
#ifdef AsciiMatrix16color
				unsigned int r = 0x7F + 0x7 + (((text.color[idx] & 0b1111) >> 3) << 6);
				unsigned int g = 0x7F + 0x7 + (((text.color[idx] & 0b0111) >> 1) << 5);
				unsigned int b = 0x7F + 0xF + ((text.color[idx] & 0b0001) << 6);
#else
#ifdef AsciiMatrixMatrixcolor
				//unsigned int r = 64, g = 196, b = 64;
				unsigned int r = 16 + 32 * text.color[idx];
				unsigned int g = 111 + 48 * text.color[idx];
				unsigned int b = 16 + 32 * text.color[idx];
#else
				unsigned int r = 0x7F + 0x7 + (((text.color[idx] & 0xE0) >> 5) << 4);
				unsigned int g = 0x7F + 0x7 + (((text.color[idx] & 0x1C) >> 2) << 4);
				unsigned int b = 0x7F + 0xF + ((text.color[idx] & 0x03) << 5);
#endif
#endif
				unsigned int imgidx = (y*BitFontHeight)*imgw + x*BitFontWidth;
				for (int i = 0; i < BitFontHeight; i++, imgidx += imgw - BitFontWidth){
					for (int j = 0; j < BitFontWidth; j++, imgidx++){
						unsigned fi = (imgidx << 1) + imgidx;
						pixelPtr[fi++] = (font.characters[text.text[idx]].bitmap[i][j] * r) >> 8;
						pixelPtr[fi++] = (font.characters[text.text[idx]].bitmap[i][j] * g) >> 8;
						pixelPtr[fi] = (font.characters[text.text[idx]].bitmap[i][j] * b) >> 8;
					}
				}
			}
		}
	}
	else{
		for (int y = 0, idx = 0; y < text.h; y++){
			for (int x = 0; x < text.w; x++, idx++){
				int yH = y*BitFontHeight, xW = x*BitFontWidth;
				for (int i = 0; i < BitFontHeight; i++){
					for (int j = 0; j < BitFontWidth; j++){
						pixelPtr[(yH + i)*frame.cols + xW + j] = font.characters[text.text[idx]].bitmap[i][j];
					}
				}
			}
		}
	}
}

