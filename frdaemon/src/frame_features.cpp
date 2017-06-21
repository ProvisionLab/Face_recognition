#include "frame_features.hpp"

#if TEST_FEATURES_COUNT

#include <random>

static std::mt19937 g_rng(std::random_device{}());

#endif // TEST_FEATURES_COUNT

std::vector<float> PersonFeatures::generate_features(cv::Mat const & sample)
{
	// 2do: generate features for sample image

#if TEST_FEATURES_COUNT

	std::vector<float> ffs(TEST_FEATURES_COUNT);

	for (int k = 0; k < TEST_FEATURES_COUNT; ++k)
	{
		ffs[k] = std::uniform_real_distribution<float>(0, 100)(g_rng);
	}

	return ffs;

#endif // TEST_FEATURES_COUNT
}

#if TEST_PERSONS_COUNT && TEST_FEATURES_COUNT
void PersonFeatures::generate_random()
{
	int n = std::uniform_int_distribution<int>(1, 2)(g_rng);

	for (int i = 0; i < n; ++i)
	{
		std::vector<float> ffs(TEST_FEATURES_COUNT);

		for (int k = 0; k < TEST_FEATURES_COUNT; ++k)
		{
			ffs[k] = std::uniform_real_distribution<float>(0, 100)(g_rng);
		}

		features.push_back(ffs);
	}

}
#endif // TEST_PERSONS_COUNT

void PersonFeatures::append_sample(cv::Mat const & sample)
{
	auto fs = generate_features(sample);
	if (!fs.empty())
		features.push_back(std::move(fs));
}

PersonFeaturesSet::PersonFeaturesSet(std::vector<std::shared_ptr<PersonFeatures>> const & ps)
	: persons(ps)
{

}

PersonFeaturesSet::PersonFeaturesSet(PersonFeaturesSet && pfs)
	: persons(std::move(pfs.persons))
{

}

PersonFeaturesSet & PersonFeaturesSet::operator = (PersonFeaturesSet && pfs)
{
	if (this != &pfs)
	{
		persons = std::move(pfs.persons);
	}

	return *this;
}

std::shared_ptr<PersonFeatures>	PersonFeaturesSet::find_nearest(std::vector<float> const & frame_feature) const
{
	float min_dist;
	std::shared_ptr<PersonFeatures> min_person;

	for (auto & person : persons)
	{
		for (auto & pfs : person->features)
		{
			float distance = compare(frame_feature, pfs);
			if (distance < found_threshold && (!min_person || distance < min_dist))
			{
				min_person = person;
				min_dist = distance;
			}
		}
	};

	return min_person;
}

float PersonFeaturesSet::compare(std::vector<float> const & frame_features, std::vector<float> const & person_features)
{
	// 2do: check if person is contained on frame

	float d2 = 0;
	for (int i = 0; i < frame_features.size() && i < person_features.size(); ++i)
	{
		float dx = frame_features[i] - person_features[i];
		d2 += dx * dx;
	}

	return d2;
}

FrameFeatures::FrameFeatures()
{

}

void FrameFeatures::generate_features(cv::Mat const & m)
{
	// 2do: generate frame features from image

#if TEST_FEATURES_COUNT

	int n = std::uniform_int_distribution<int>(0, 3)(g_rng);

	for (int i = 0; i < n; ++i)
	{
		std::vector<float> ffs(TEST_FEATURES_COUNT);

		for (int k = 0; k < TEST_FEATURES_COUNT; ++k)
		{
			ffs[k] = std::uniform_real_distribution<float>(0, 100)(g_rng);
		}

		features.push_back(ffs);
	}
#endif // TEST_FEATURES_COUNT
}

std::set<std::shared_ptr<PersonFeatures>> FrameFeatures::compare_persons(PersonFeaturesSet const & ps)
{
	std::set<std::shared_ptr<PersonFeatures>> found_persons;

	for (auto & ffs : features)
	{
		auto person = ps.find_nearest(ffs);

		if (person)
		{
			found_persons.insert(person);
		}
	}

	return found_persons;
}

