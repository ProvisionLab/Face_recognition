#ifndef TEXT_DETECTION_H
#define TEXT_DETECTION_H
/*
 * textdetection.cpp
 *
 * A demo program of End-to-end Scene Text Detection and Recognition:
 * Shows the use of the Tesseract OCR API with the Extremal Region Filter algorithm described in:
 * Neumann L., Matas J.: Real-Time Scene Text Localization and Recognition, CVPR 2012
 *
 * Created on: Jul 31, 2014
 *     Author: Lluis Gomez i Bigorda <lgomez AT cvc.uab.es>
 */

#include "opencv2/text.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include <iostream>

using namespace std;
using namespace cv;
using namespace cv::text;
//Perform text detection and recognition and evaluate results using edit distance
int tess(Mat image);

size_t edit_distance(const string &A, const string &B);

bool isRepetitive(const string &s);
void er_draw(vector<Mat> &channels, vector<vector<ERStat> > &regions, vector<Vec2i> group, Mat &segmentation);

size_t min(size_t x, size_t y, size_t z);
#endif // TEXT_DETECTION_H