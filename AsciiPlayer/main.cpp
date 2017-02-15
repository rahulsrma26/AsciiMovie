/*
	Author:	Rahul Sharma
	Date  : 12 Dec'16
*/

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>

const std::string playerWindowName = "ASCII Player";
const std::string paramHelp = 
	"asciiplayer <inputFile> [<option> <value>]"
	"\n <inputFile> input file path"
	"\n options:"
	"\n -i <outputFolderPath>   output converted images"
	"\n -v <outputFilePath>     output converted video";

int main(int argc, char* argv[]){
	using namespace std;
	cout << "openCV version : " << CV_VERSION << '\n';

	string fileName(argc > 1 ? argv[1] : "..\\Videos\\test6.mp4");
	string videoOutputFileName = "out.avi", imagesOutputFolder = "..\\out";
	for (int i = 2; i < argc; i += 2) {
		string option(argv[i]);
		if (option == "-i")
			imagesOutputFolder = string(argv[i + 1]);
		else if (option == "-v")
			videoOutputFileName = string(argv[i + 1]);
		else {
			cout << paramHelp << '\n';
			return 1;
		}
	}

	cv::VideoCapture video(fileName);
	if (!video.isOpened()) {
		cout << "Can't open file '" << fileName << "'\n";
		return 2;
	}

	cv::namedWindow(playerWindowName, CV_WINDOW_AUTOSIZE | CV_WINDOW_KEEPRATIO | CV_GUI_EXPANDED);
	while(true){
		cv::Mat frame;
		if (!video.read(frame))
			break;	// EOF
		
		cv::imshow(playerWindowName, frame);
		if (cv::waitKey(1) == 27)
			break;
	}
	cv::destroyWindow(playerWindowName);
}