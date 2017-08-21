#pragma once

#include <opencv2/opencv.hpp>
#include <memory>

#include "cards_recognition\convolution_detector.h"
#include "cards_recognition\kmeans.h"
#include "cards_recognition\black_jack.h"
#include "cards_recognition\baccarat.h"

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
		std::pair<int,std::vector<Card> >	cards;
		std::string to_json() const;
	};

	~CardsTable();

	bool set_config(std::string const & jsonConfig);

	bool init();
	void uninit();

	std::vector<Results> recognize(cv::Mat const & frame);
	void processBlackJack(std::vector<Card> & detection_results, cv::Mat & src);
	void processBaccarat(Results & detection_results, cv::Mat & src, int id);
protected:

	cv::Mat get_area(cv::Mat const & frame, CropArea const & area) const;

protected:

	int TableNumber = 0;
	int SourceType = 0;
	std::string	Game;
	std::vector<CropArea>	Crops;
	std::vector<CropArea>	Buttons;
	std::vector<Results> current_results;
	std::unique_ptr<ConvolutionDetector> detector;
	bool may_be_split, split, bj, hasChip;
	int steps;
};
