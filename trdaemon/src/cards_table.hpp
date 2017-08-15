#pragma once

#include <opencv2/opencv.hpp>

class CardsTable
{
public:

	bool set_config(std::string const & json);

	std::string recognize(cv::Mat const & frame);

};
