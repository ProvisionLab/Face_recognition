#pragma once

#include <list>
#include <vector>
#include <set>
#include <memory>
#include <functional>

#include "recognition/TestFaceDetection.inc.h"
#include "recognition/caffe_person_recognition.h"
#include "recognition/util/help_functions.h"

#include <opencv2/opencv.hpp>

#define SOLUTION_VERSION		4
#define SOLUTION_FEATURES_COUNT	4096u

//#define TEST_USE_RANDOM_FEATURES	1
// uncomment this for generate virtual persons with random features
//#define TEST_USE_PERSONS_COUNT		10
//#define TEST_USE_SAMPLES_COUNT		15
//#define TEST_USE_ALT_FNN			1

class PersonFeatures
{
public:

	virtual ~PersonFeatures()
	{
	}

	void append_sample(cv::Mat const & sample);

#if TEST_USE_PERSONS_COUNT
	void generate_random();
#endif

public:

	std::list<std::vector<float>> features;

private:

	static std::vector<float> generate_features(cv::Mat const & sample);
};

class PersonFeaturesSet
{
public:

	PersonFeaturesSet() {}

	PersonFeaturesSet(std::vector<std::shared_ptr<PersonFeatures>> const & ps);

	PersonFeaturesSet(PersonFeaturesSet && pfs);

	PersonFeaturesSet & operator = (PersonFeaturesSet && pfs);

	std::shared_ptr<PersonFeatures>	find_nearest(std::vector<float> const & frame_features) const;

#if TEST_USE_ALT_FNN
	std::shared_ptr<PersonFeatures>	find_nearest_alt(std::vector<float> const & frame_features) const;
#endif

public:

	std::vector<std::shared_ptr<PersonFeatures>>	persons;

private:

	const float found_threshold = 500000.0f;

	static float compare(std::vector<float> const & frame_features, std::vector<float> const & person_features);
};

class FrameFeatures
{
public:

	FrameFeatures();

	void generate_features(cv::Mat const & m);

	std::set<std::shared_ptr<PersonFeatures>> compare_persons(PersonFeaturesSet const & ps);

	static void initialize();
	
	static cv::Mat target_mat;
	static boost::shared_ptr<FaceInception::CascadeCNN> detector;
	static boost::shared_ptr<CaffeDetector> recognizer;

private:

	std::list<std::vector<float>>	features;

	std::vector<std::vector<cv::Point2f> > src_points;
	std::vector<std::vector<cv::Point2d>> points;

	std::set<std::shared_ptr<PersonFeatures>>	found_persons;

};

