#pragma once

#include <list>
#include <vector>
#include <set>
#include <memory>

#include <opencv2/opencv.hpp>

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

	void compare_person(std::shared_ptr<PersonFeatures> person);

	std::set<std::shared_ptr<PersonFeatures>> const & get_found_persons();

private:

	float compare(std::vector<float> const & person_features);

	void append_distance(float dist, std::shared_ptr<PersonFeatures> person);

	std::list<std::vector<float>>	features;

	const float found_threshold = 1.0f;

	std::set<std::shared_ptr<PersonFeatures>>	found_persons;
};
