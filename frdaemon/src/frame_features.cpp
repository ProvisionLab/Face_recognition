#include "frame_features.hpp"

FrameFeatures::FrameFeatures()
{

}

void FrameFeatures::generate_features(cv::Mat const & m)
{
	// 2do: generate frame features from image
}

bool FrameFeatures::contains_person(std::list<std::vector<float>> const & person_features)
{
	// 2do: check if person is contained on frame
	return false;
}

std::vector<float> generate_features_for_sample(cv::Mat const & sample)
{
	// 2do: download from ftp samples & generate features
	return { SOLUTION_VERSION, 1, 1, 1, 1 };
}
