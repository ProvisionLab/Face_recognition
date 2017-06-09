#pragma once

#include <list>
#include <vector>

#include <opencv2/opencv.hpp>
#include "person.hpp"

class FrameFeatures
{
public:

	FrameFeatures();

	void generate(cv::Mat const & m);

	bool contains_person(Person const & person);

private:

	std::list<std::vector<float>>	features;
};






