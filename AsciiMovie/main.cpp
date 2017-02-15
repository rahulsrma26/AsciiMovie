#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include "BitFont.h"
#include "AmpAscii.h"

using namespace std;
using namespace cv;

VideoCapture *video;
int currentFrame = 0;
double videoFPS;
bool trackbarAutoCall = false;
void frameSkip(int, void*){
	if (trackbarAutoCall){
		trackbarAutoCall = false;
		return;
	}
	bool success = video->set(CAP_PROP_POS_MSEC, 1000.0*currentFrame / videoFPS);
	if (!success)
		std::cout << "Cannot set frame position from video file at " << std::endl;
}

int main(int argc, char* argv[]){
	BitFont font;
	font.readFont("..\\Fonts\\limited8x12blur180.bmp");
	font.normalize();
	AmpFontStream* fontStream = getAmpFontStream(font);

	video = new VideoCapture("..\\Videos\\matrix.mp4"); // open the video file for reading
	VideoCapture& cap = *video;
	if (!cap.isOpened()) {
		cout << "Cannot open the video file" << endl;
		system("pause");
		return -1;
	}

	VideoWriter outputVideo;
	bool exportVideo = false;
	bool video = true;
	bool ascii = true;
	bool exportImage = false;

	vector<int> imageParams;
	imageParams.push_back(CV_IMWRITE_JPEG_QUALITY);
	imageParams.push_back(85);

	if (exportVideo) {
		Size inputSize = Size((int)cap.get(CV_CAP_PROP_FRAME_WIDTH), (int)cap.get(CV_CAP_PROP_FRAME_HEIGHT));
		string outputFileName = "..\\Videos\\out.mp4";
		outputVideo.open(outputFileName, -1, cap.get(CV_CAP_PROP_FPS), inputSize, true);
		if (!outputVideo.isOpened()){
			cout << "Could not open the output video for write: " << outputFileName << endl;
			return -1;
		}
	}
	
	int lastFrame = cap.get(CAP_PROP_FRAME_COUNT);
	videoFPS = cap.get(CAP_PROP_FPS); //get the frames per seconds of the video
	cout << "Frame per seconds : " << videoFPS << endl;
	if (video) {
		namedWindow("MyVideo", WINDOW_AUTOSIZE | WINDOW_KEEPRATIO); //create a window called "MyVideo"
		createTrackbar("Frame", "MyVideo", &currentFrame, lastFrame, frameSkip);
	}
	else {
		namedWindow("MyVideo", WINDOW_AUTOSIZE | WINDOW_NORMAL);
		resizeWindow("MyVideo", 256, 256);
	}
	cap.set(CAP_PROP_POS_FRAMES, currentFrame);

	while (1){
		Mat frame;
		if (!cap.read(frame)) {
			cout << "Cannot read the frame from video file. EOF." << endl;
			break;
		}
		//Rect roi(0, 120, 1440, 840);
		//Mat frame = rawframe(roi);
		if (video) {
			trackbarAutoCall = true;
			setTrackbarPos("Frame", "MyVideo", currentFrame);
		}
		
		//blur(frame, frame, Size(1, 1), Point(-1, -1));
		Mat src = frame.clone();
		bilateralFilter(src, frame, 16, 32, 32);

		//resize(frame, frame, Size(frame.cols * 1.5, frame.rows * 1.5), 1.5, 1.5, INTER_LINEAR);
		Mat bwFrame;
		if (ascii) {
			cvtColor(frame, bwFrame, COLOR_BGR2GRAY);
			//TextBlock* tb = imageToAscii(*fontStream, bwFrame, 16);
			//genFrameFromText(bwFrame, font, *tb, false);
			Mat blockFrame;
			resize(frame, blockFrame, Size(frame.cols / BitFontWidth, frame.rows / BitFontHeight), BitFontWidth, BitFontHeight, INTER_LINEAR);
			TextBlock* tb = imageToAscii(*fontStream, bwFrame, 32, &blockFrame);
			//TextBlock* tb = imageToAscii2(font, bwFrame, 32, &blockFrame);
			frame = Scalar(0, 0, 0);
			genFrameFromText(frame, font, *tb, true);
			//genFrameFromText(bwFrame, font, *tb, false);
			delete[] tb->text;
			delete tb;
			//resize(frame, frame, Size(frame.cols * 2, frame.rows * 2), 2.0, 2.0, INTER_LANCZOS4);
		}

		if(video)
			imshow("MyVideo", frame);
		
		if (exportVideo)
			outputVideo << frame;
		
		if (exportImage) {
			char fname[100];
			sprintf(fname, "..\\out\\o%06d.jpg", currentFrame);
			imwrite(fname, frame, imageParams);
		}

		if (!(currentFrame & 3))
			printf("%2.2f%%. time (in sec): %5.2f \r", 100.0*currentFrame/lastFrame, currentFrame/videoFPS);
		
		if (waitKey(1) == 27){
			cout << "esc key is pressed by user" << endl;
			break;
		}
		currentFrame++;
	}

	return 0;
}