#pragma once
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <stdint.h>


namespace OCV
{

	/**
	* @brief   get the mass of the corners
	* @param   corners input corners
	* @return  the centers of the corners
	*/
	template<typename T>
	inline T corners_center(std::vector<T> const &corners)
	{
		return std::accumulate(std::begin(corners), std::end(corners), cv::Point(0, 0)) * (1. / corners.size());
	}

	/**
	*@brief convert the points between cv::Point_<T> and cv::Point_<U>.
	*@param src input points
	*@param dst the points after convert from src
	*/
	template<typename T, typename U>
	void convert_points(std::vector<T> const &src, std::vector<U> &dst)
	{
		dst.clear();
		for (auto const &point : src){
			dst.emplace_back(point.x, point.y);
		}
	}

	/**
	* @brief sort the corners of the rectangle to top left, top right, bottom right, bottom left.
	* users have to make sure the points of the corners are valid(is a rect), else the results
	* are undefined
	* @param corners input corners
	*
	*/

	template<typename T>
	void sort_rect_corners(std::vector<T> &corners)
	{
		std::vector<T> top, bot;
		T tl, tr, bl, br;
		T const center = corners_center(corners);
		for (size_t i = 0; i < corners.size(); i++){
			if (corners[i].y <= center.y)
				top.emplace_back(corners[i]);
			else
				bot.emplace_back(corners[i]);
		}

		if (top.size() == 2 && bot.size() == 2)
		{
			tl = top[0].x > top[1].x ? top[1] : top[0];
			tr = top[0].x > top[1].x ? top[0] : top[1];
			bl = bot[0].x > bot[1].x ? bot[1] : bot[0];
			br = bot[0].x > bot[1].x ? bot[0] : bot[1];
		}

		float height = bl.y - tl.y;
		float width = tr.x - tl.x;
		height = fabs(height);
		width = fabs(width);

		if (width > height)
		{
			T tmp_tl = tl;
			T tmp_tr = tr;
			T tmp_br = br;
			T tmp_bl = bl;
			tl = tmp_bl;
			tr = tmp_tl;
			br = tmp_tr;
			bl = tmp_br;
		}

		corners.clear();
		corners.emplace_back(tl);
		corners.emplace_back(tr);
		corners.emplace_back(br);
		corners.emplace_back(bl);
	}

}

/**
* @brief find the contours of images
* @param binary_input binary input image
* @return the contours
*/
std::vector<std::vector<cv::Point> > find_contours(cv::Mat const &binary_input)
{
	std::vector<std::vector<cv::Point> > contours;
	cv::findContours(binary_input, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

	return contours;
}

/**
* @brief get the corners of the quadrilaterals
* @param contours input contours
* @return the corners of the quadrilaterals
*/
std::vector<std::vector<cv::Point> > get_corners(std::vector<std::vector<cv::Point> > &contours)
{
	//OCV::remove_contours(contours, 100, 50000);
	std::vector<cv::Point> approx;
	std::vector<std::vector<cv::Point> > corners;
	for (auto const &data : contours){
		cv::approxPolyDP(data, approx, cv::arcLength(data, true) * 0.02, true);
		if (approx.size() == 4 && cv::isContourConvex(approx)){
			OCV::sort_rect_corners(approx);
			corners.emplace_back(std::move(approx));
		}
	}

	return corners;
}

int32_t match_mat(cv::Mat A, cv::Mat B, int skip)
{
	int32_t ret = 0;
	if (A.rows != B.rows) return INT32_MAX;
	if (A.cols != B.cols) return INT32_MAX;
	for (int i = 0; i < A.rows; i += skip)
	{
		for (int j = 0; j < A.cols; j += skip)
		{
			ret += abs(A.at<uchar>(i, j) - B.at<uchar>(i, j));
		}
	}
	return ret;
}


