#pragma once
#include <utility>  
#include <vector>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "convolution_detector.h"

std::pair<cv::Point2f, bool> gotChip(cv::Mat& image, int id, bool& hasChip, int& steps);
int checkBet(int height, cv::Point2f& center);