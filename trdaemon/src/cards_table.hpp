#pragma once

#include <opencv2/opencv.hpp>

class CardsTable
{
public:

	struct CropArea
	{
		int AreaNumber = 0;
		float Left = 0;
		float Top = 0;
		float Right = 0;
		float Bottom = 0;

		cv::Rect get_rect(cv::Size img_size) const;
	};

	struct Results
	{
		// 2do: fields

		std::string to_json() const;
	};

	bool set_config(std::string const & jsonConfig);

	bool init();

	Results recognize(cv::Mat const & frame);

protected:

	cv::Mat get_area(cv::Mat const & frame, CropArea const & area) const;

protected:

	int TableNumber = 0;
	int SourceType = 0;
	std::string	Game;
	std::vector<CropArea>	Crops;
	std::vector<CropArea>	Buttons;
};
