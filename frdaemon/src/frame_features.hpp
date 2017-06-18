#pragma once

#include <list>
#include <vector>

#include <opencv2/opencv.hpp>

#include <boost/shared_ptr.hpp>

#include "recognition/TestFaceDetection.inc.h"
#include "recognition/caffe_person_recognition.h"
#include "recognition/util/help_functions.h"

#define SOLUTION_VERSION	1

class FrameFeatures
{
public:

	FrameFeatures();

	void generate_features(cv::Mat const & m);

	bool contains_person(std::list<std::vector<float>> const & person_features);

	static void initialize();

	static cv::Mat target_mat;
	
	static boost::shared_ptr<FaceInception::CascadeCNN> detector;
	static boost::shared_ptr<CaffeDetector> recognizer;
private:

	std::list<std::vector<float>>	features;


	std::vector<std::vector<cv::Point2f> > src_points;
	std::vector<std::vector<cv::Point2d>> points;



};

std::vector<float> generate_features_for_sample(cv::Mat const & sample);

