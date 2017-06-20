#pragma once

#include <list>
#include <vector>
#include <set>
#include <memory>

#include <opencv2/opencv.hpp>

#include <boost/shared_ptr.hpp>

#include "recognition/TestFaceDetection.inc.h"
#include "recognition/caffe_person_recognition.h"
#include "recognition/util/help_functions.h"

#define SOLUTION_VERSION	1

class PersonFeatures
{
public:

	virtual ~PersonFeatures()
	{
	}

	void append_sample(cv::Mat const & sample);

public:

	std::list<std::vector<float>> features;

private:

	static std::vector<float> generate_features(cv::Mat const & sample);
};

class FrameFeatures
{
public:

	FrameFeatures();

	void generate_features(cv::Mat const & m);

	void compare_persons(std::list<std::shared_ptr<PersonFeatures>> const & persons);

	std::set<std::shared_ptr<PersonFeatures>> const & get_found_persons();

	static void initialize();

	static cv::Mat target_mat;
	
	static boost::shared_ptr<FaceInception::CascadeCNN> detector;
	static boost::shared_ptr<CaffeDetector> recognizer;
private:

	float compare(std::vector<float> const & person_features);

	std::list<std::vector<float>>	features;

	std::vector<std::vector<cv::Point2f> > src_points;
	std::vector<std::vector<cv::Point2d>> points;

	const float found_threshold = 1.0f;

	std::set<std::shared_ptr<PersonFeatures>>	found_persons;

};

std::vector<float> generate_features_for_sample(cv::Mat const & sample);

