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

void FrameFeatures::compare_persons(std::list<std::shared_ptr<PersonFeatures>> const & persons)
{
	std::vector<std::pair<float, std::shared_ptr<PersonFeatures>>> dists;

	for (auto & person : persons)
	{
		for (auto & f : person->features)
		{
			float dist = compare(f);
			dists.push_back(std::make_pair(dist, person));
		}
	}

	auto min_pair = std::min_element(dists.begin(), dists.end(), [](decltype(dists)::value_type &a, decltype(dists)::value_type &b) ->bool
	{
		return a.first < b.first;
	});

	found_persons.insert(min_pair->second);
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
