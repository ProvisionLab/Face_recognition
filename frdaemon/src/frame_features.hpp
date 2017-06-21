#pragma once

#include <list>
#include <vector>
#include <set>
#include <memory>
#include <functional>

#include <opencv2/opencv.hpp>

#define SOLUTION_VERSION	1

//#define TEST_PERSONS_COUNT	1000
#define TEST_FEATURES_COUNT	15

class PersonFeatures
{
public:

	virtual ~PersonFeatures()
	{
	}

	void append_sample(cv::Mat const & sample);

#if TEST_PERSONS_COUNT && TEST_FEATURES_COUNT
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

	std::shared_ptr<PersonFeatures>	find_nearest(std::vector<float> const & frame_feature) const;

public:

	std::vector<std::shared_ptr<PersonFeatures>>	persons;

private:

	const float found_threshold = 5000.0f;
	static float compare(std::vector<float> const & frame_features, std::vector<float> const & person_features);
};

class FrameFeatures
{
public:

	FrameFeatures();

	void generate_features(cv::Mat const & m);

	std::set<std::shared_ptr<PersonFeatures>> compare_persons(PersonFeaturesSet const & ps);

private:

	std::list<std::vector<float>>	features;

};

