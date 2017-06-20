#include "frame_features.hpp"

#include <random>

std::vector<float> PersonFeatures::generate_features(cv::Mat const & sample)
{
	// 2do: generate features for sample image

	return { SOLUTION_VERSION, 1, 1, 1, 1 };
}

void PersonFeatures::append_sample(cv::Mat const & sample)
{
	auto fs = generate_features(sample);
	if (!fs.empty())
		features.push_back(std::move(fs));
}

FrameFeatures::FrameFeatures()
{

}

void FrameFeatures::generate_features(cv::Mat const & m)
{
	// 2do: generate frame features from image
}

void FrameFeatures::compare_person(std::shared_ptr<PersonFeatures> person)
{
	for (auto & f : person->features)
	{
		append_distance(compare(f), person);
	}
}

void FrameFeatures::append_distance(float dist, std::shared_ptr<PersonFeatures> person)
{
	if (dist < found_threshold)
	{
		found_persons.insert(person);
	}
}

std::set<std::shared_ptr<PersonFeatures>> const & FrameFeatures::get_found_persons()
{
	return found_persons;
}

float FrameFeatures::compare(std::vector<float> const & person_features)
{
	// 2do: check if person is contained on frame
	static std::default_random_engine rng(std::random_device{}());

	return std::uniform_real_distribution<float>(0, 100)(rng);
}
