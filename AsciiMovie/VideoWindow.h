#pragma once
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <string>

class VideoWindow{
	std::string _name;
	std::string _filePath;
	cv::VideoCapture *_videoFile;
	int _currentFrameNumber;
	int _lastFrameNumber;
	bool _trackbarAutoCall;
	double _videoFPS;
	cv::Mat _currentFrameImage;
public:

	VideoWindow(const std::string &windowName);

	bool init(const std::string &filePath, bool isTrackBar = true);

	static void frameSkip(int newValue, void* object);

	cv::Mat* getNextFrame();
	void showFrame(cv::Mat *frame = nullptr);

	double getVideoFPS() const;
	int getCurrentFrameNumber() const;
};

