#include <iostream>

#include <alpr.h>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#pragma once
#include <opencv2/calib3d/calib3d.hpp>

#include "text_detection.hpp"
#include <regex>


using namespace std;

//#define STEPS


// Initialize the library using United States style license plates.
// You can use other countries/regions as well (for example: "eu", "au", or "kr")

void init_alpr();

std::string exec(const char* cmd);

void printPlate(cv::Mat& plate);
std::string printPlateAlpr(cv::Mat& plate);

void showPlate(cv::RotatedRect& mr, cv::Mat& image);
std::string cropPlate(cv::RotatedRect& mr, cv::Mat& image);

void processImage(cv::Mat& image, cv::Mat& processed_image);

std::vector<std::string> plate_recognition(cv::Mat& image);