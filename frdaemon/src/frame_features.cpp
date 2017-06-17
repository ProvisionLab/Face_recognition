#include "frame_features.hpp"

#include <random>

std::vector<float> generate_features_for_sample(cv::Mat const & sample)
{
	// 2do: generate features for sample image
	return{ SOLUTION_VERSION, 1, 1, 1, 1 };
}

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

	static std::default_random_engine rng(std::random_device{}());

	return std::uniform_real_distribution<float>(0,100)(rng) < 2.0;

}
