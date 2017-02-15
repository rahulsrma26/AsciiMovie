#include "VideoWindow.h"

VideoWindow::VideoWindow(const std::string &windowName)
	: _name(windowName), _currentFrameNumber(0), _trackbarAutoCall(false){
	cv::namedWindow(_name, cv::WINDOW_AUTOSIZE | cv::WINDOW_KEEPRATIO);
}

bool VideoWindow::init(const std::string &filePath, bool isTrackBar){
	_filePath = filePath;
	_videoFile = new cv::VideoCapture(_filePath.c_str());
	_lastFrameNumber = _videoFile->get(cv::CAP_PROP_FRAME_COUNT);
	_videoFPS = _videoFile->get(cv::CAP_PROP_FPS);

	if (isTrackBar)
		cv::createTrackbar("Frame", _name.c_str(), &_currentFrameNumber, _lastFrameNumber, &VideoWindow::frameSkip, this);

	return _videoFile->isOpened();
}

void VideoWindow::frameSkip(int newValue, void* object){
	VideoWindow* window = (VideoWindow*)object;
	if (window->_trackbarAutoCall){
		window->_trackbarAutoCall = false;
		return;
	}
	bool success = window->_videoFile->set(cv::CAP_PROP_POS_MSEC, 1000.0*window->_currentFrameNumber / window->_videoFPS);
	if (!success)
		std::cout << "Cannot set frame position from video file at " << std::endl;
}

double VideoWindow::getVideoFPS() const{
	return _videoFPS;
}

int VideoWindow::getCurrentFrameNumber() const{
	return _currentFrameNumber;
}

cv::Mat* VideoWindow::getNextFrame(){
	if (_videoFile->read(_currentFrameImage)){
		_currentFrameNumber++;
		_trackbarAutoCall = true;
		cv::setTrackbarPos("Frame", _name.c_str(), _currentFrameNumber);
		return &_currentFrameImage;
	}
	return nullptr;
}

void VideoWindow::showFrame(cv::Mat *frame){
	if (frame == nullptr)
		imshow(_name.c_str(), _currentFrameImage);
	else
		imshow(_name.c_str(), *frame);
}

