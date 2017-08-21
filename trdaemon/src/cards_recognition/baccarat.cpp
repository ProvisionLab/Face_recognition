#include "baccarat.h"
std::pair<cv::Point2f, bool> gotChip(cv::Mat& image, int id, bool& hasChip, int& steps)
{

	cv::Scalar lower = cv::Scalar(29, 24, 147);
	cv::Scalar upper = cv::Scalar(49, 44, 167);

	cv::Mat mask;

	cv::inRange(image, lower, upper, mask);

	int dilation_type = cv::MORPH_RECT;
	int dilation_size = 5;

	cv::Mat element = cv::getStructuringElement(dilation_type,
		cv::Size(2 * dilation_size + 1, 2 * dilation_size + 1),
		cv::Point(dilation_size, dilation_size));

	/// Apply the erosion operation
	cv::dilate(mask, mask, element);

#ifdef CHIPS_MASK
	cv::imshow("mask" + std::to_string(id), mask);
	cv::waitKey(10);
#endif
	std::vector<std::vector<cv::Point> > contours;
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(mask, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
	double max = 0;
	std::vector<cv::Point> winner;
	int k = 0;

	int maxId = 0;
	for (size_t i = 0; i< contours.size(); i++)
	{
		auto minRect = cv::minAreaRect(contours[i]);
		if (minRect.size.area() > max)
		{
			max = minRect.size.area();
			winner = contours[i];
			maxId = i;
		}

	}
	if (contours.size() > 0)
	{
		cv::Scalar color = cv::Scalar(255, 0, 0);
		//cv::drawContours(image, contours, maxId, color, 2, 8, std::vector<cv::Vec4i>(), 0, cv::Point());

		auto minRect = cv::minAreaRect(contours[maxId]);
		//std::cout << "S=" << minRect.size.area() << std::endl;
		if (minRect.size.area() > 10000 && minRect.size.area() < 15000)
		{
			hasChip = true;
			steps++;
		}
		else
		{
			hasChip = false;
			steps = 0;
		}
	}
	else
	{
		steps = 0;
		hasChip = false;
	}

	if (hasChip && steps > 5)
	{
		cv::Moments mu;
		mu = cv::moments(contours[maxId], true);
		cv::Point2f center;
		center = cv::Point2f(mu.m10 / mu.m00, mu.m01 / mu.m00);
		cv::circle(image, center, 4, cv::Scalar(255, 0, 0), -1, 8, 0);
		return std::make_pair(center, true);
	}
	else
	{
		return std::make_pair(cv::Point2f(0, 0), false);
	}
}



int checkBet(int height, cv::Point2f& center)
{
	int deriver = height / 3;
	if (center.y < deriver)
	{
		return 1;
	}
	if (center.y > deriver && center.y < deriver * 2)
	{
		return 2;
	}
	if (center.y > deriver * 2)
	{
		return 0;
	}
}
