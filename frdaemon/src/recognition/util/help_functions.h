#ifndef HELP_FUNCTIONS_H
#define HELP_FUNCTIONS_H
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <iostream>

void faceTransform(cv::Mat& face, cv::Mat& warp_dst, std::vector<cv::Point2f> &src_points);

void faceTransformRegard(cv::Mat& face, cv::Mat& warp_dst, std::vector<cv::Point2f> &src_points, cv::Rect_<double>& src_rect, cv::Mat target_mat);

float euclidianDist(const cv::Point2f& p1, const cv::Point2f& p2);

#endif // HELP_FUNCTIONS_H
