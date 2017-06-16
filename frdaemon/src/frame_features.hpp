#pragma once

#include <list>
#include <vector>

#include <opencv2/opencv.hpp>

#define SOLUTION_VERSION	1

class FrameFeatures
{
public:

	FrameFeatures();

	void generate_features(cv::Mat const & m);

	bool contains_person(std::list<std::vector<float>> const & person_features);

private:

	std::list<std::vector<float>>	features;
};

std::vector<float> generate_features_for_sample(cv::Mat const & sample);
