#include <iostream>

#include <alpr.h>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include "opencv2/text.hpp"
#include "opencv2/core/utility.hpp"

using namespace cv::text;

#include <regex>

using namespace std;
using namespace cv;
using namespace cv::text;

//#define STEPS

class PlateRecognizer
{
public:
	PlateRecognizer();
	~PlateRecognizer();

	static void init();

	std::vector<std::pair<std::string, cv::Mat>> recognize(cv::Mat const & frame);

private: 

	cv::Mat processImage();
	void processPlate(cv::RotatedRect & mr);
	void definePlate(cv::Mat & plate);

	static Ptr<ERFilter> er_filter1;
	static Ptr<ERFilter> er_filter2;
	static Ptr<OCRTesseract> ocr;
	static std::unique_ptr<alpr::Alpr> openalpr;
	static std::string current_res;

	cv::Mat image_;
	std::vector<alpr::AlprRegionOfInterest> rois;
	std::vector<std::pair<std::string, cv::Mat>> results_;

};

