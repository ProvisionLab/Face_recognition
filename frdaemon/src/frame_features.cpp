#include "frame_features.hpp"

#if TEST_USE_PERSONS_COUNT
#include <iostream>
#endif

#if TEST_USE_RANDOM_FEATURES
#include <random>
static std::mt19937 g_rng(std::random_device{}());
#endif

std::vector<float> PersonFeatures::generate_features(cv::Mat const & sample)
{
	// 2do: generate features for sample image
	cv::Mat image = sample.clone();
	std::vector<std::vector<cv::Point2f> > src_points;
	std::vector<std::vector<cv::Point2d>> points;
    double min_face_size = 40;
    auto result = FrameFeatures::detector->GetDetection(image, 12 / min_face_size, 0.7, true, 0.7, true, points);


    for (int i = 0; i < result.size(); i++)
    {
        cv::Size_<double> deltaSize( result[i].first.width * 0.1f, result[i].first.height * 0.1f );
        cv::Point_<double> offset( deltaSize.width/2, deltaSize.height/2);
        result[i].first -= deltaSize;
        result[i].first += offset;

        std::vector<cv::Point2f> temp_points;

        for (int p = 0; p < 5; p++)
        {
            if (p!=2)
                temp_points.push_back(points[i][p]);
        }

        src_points.push_back(temp_points);

    }

    cv::Mat face;
    std::vector<float> current_features;
    for(int i=0 ; i< result.size(); i++)
    {

        std::chrono::time_point<std::chrono::system_clock> p3 = std::chrono::system_clock::now();

        faceTransformRegard(image,face,src_points[i],result[i].first,FrameFeatures::target_mat);

        current_features=std::move(FrameFeatures::recognizer->Detect(face));

    }

#if TEST_USE_RANDOM_FEATURES

	std::vector<float> ffs(SOLUTION_FEATURES_COUNT);

	for (int k = 0; k < SOLUTION_FEATURES_COUNT; ++k)
	{
		ffs[k] = std::uniform_real_distribution<float>(0, 100)(g_rng);
	}

	return ffs;

#endif // TEST_USE_RANDOM_FEATURES

	return current_features;
}

#if TEST_USE_PERSONS_COUNT
void PersonFeatures::generate_random()
{
	int n = std::uniform_int_distribution<int>(1, 2)(g_rng);

	for (int i = 0; i < n; ++i)
	{
		std::vector<float> ffs(SOLUTION_FEATURES_COUNT);

		for (int k = 0; k < SOLUTION_FEATURES_COUNT; ++k)
		{
			ffs[k] = std::uniform_real_distribution<float>(0, 100)(g_rng);
		}

		features.push_back(ffs);
	}

}
#endif // TEST_USE_PERSONS_COUNT

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

std::shared_ptr<PersonFeatures>	PersonFeaturesSet::find_nearest(std::vector<float> const & frame_features) const
{
	float min_dist = 10000;
	std::shared_ptr<PersonFeatures> min_person;

	for (auto & person : persons)
	{
		std::cout<<"person check start"<<std::endl;
		for (auto & pfs : person->features)
		{
			std::cout<<"Persons features check start"<<std::endl;
			float distance = compare(frame_features, pfs);
			if (distance < found_threshold && (!min_person || distance < min_dist))
			{
				min_person = person;
				min_dist = distance;
			}
		}
	};
	std::cout<<"Min dist="<<min_dist<<std::endl;
	return min_person;
}

#if TEST_USE_ALT_FNN
std::shared_ptr<PersonFeatures>	PersonFeaturesSet::find_nearest_alt(std::vector<float> const & frame_features) const
{
	return find_nearest(frame_features);
}
#endif // TEST_USE_ALT_FNN

float PersonFeaturesSet::compare(std::vector<float> const & frame_features, std::vector<float> const & person_features)
{
	// 2do: check if person is contained on frame

	float d2 = 0;
	for (int i = 0; i < (int)frame_features.size() && i < (int)person_features.size(); ++i)
	{
		float dx = frame_features[i] - person_features[i];
		d2 += dx * dx;
	}
	std::cout<<"COMPARE RESULT="<<d2<<std::endl;
	return d2;
}

FrameFeatures::FrameFeatures()
{

}

void FrameFeatures::generate_features(cv::Mat const & m)
{
	// 2do: generate frame features from image
	cv::Mat sample = m.clone();
	src_points.clear();
	points.clear();
    double min_face_size = 40;
    auto result = detector->GetDetection(sample, 12 / min_face_size, 0.7, true, 0.7, true, points);


    for (int i = 0; i < result.size(); i++)
    {
        cv::Size_<double> deltaSize( result[i].first.width * 0.1f, result[i].first.height * 0.1f );
        cv::Point_<double> offset( deltaSize.width/2, deltaSize.height/2);
        result[i].first -= deltaSize;
        result[i].first += offset;
	cv::rectangle(sample, result[i].first, cv::Scalar(255,0,0) );
        std::vector<cv::Point2f> temp_points;

        for (int p = 0; p < 5; p++)
        {
            if (p!=2)
                temp_points.push_back(points[i][p]);
        }

        src_points.push_back(temp_points);

    }
    
    cv::Mat face;

    for(int i=0 ; i< result.size(); i++)
    {

        std::chrono::time_point<std::chrono::system_clock> p3 = std::chrono::system_clock::now();

        faceTransformRegard(sample,face,src_points[i],result[i].first,target_mat);

        std::vector<float> current_features(recognizer->Detect(face));

        features.push_back(current_features);

    }

    cv::resize(sample, sample, cv::Size(800,600));
    cv::imshow("dbg", sample);
    cv::waitKey(1);
#if TEST_USE_RANDOM_FEATURES

	int n = std::uniform_int_distribution<int>(0, 3)(g_rng);

	for (int i = 0; i < n; ++i)
	{
		std::vector<float> ffs(SOLUTION_FEATURES_COUNT);

		for (int k = 0; k < SOLUTION_FEATURES_COUNT; ++k)
		{
			ffs[k] = std::uniform_real_distribution<float>(0, 100)(g_rng);
		}

		features.push_back(ffs);
	}
#endif // TEST_USE_RANDOM_FEATURES
}

std::set<std::shared_ptr<PersonFeatures>> FrameFeatures::compare_persons(PersonFeaturesSet const & ps)
{
	std::set<std::shared_ptr<PersonFeatures>> found_persons;

	for (auto & ffs : features)
	{
		auto person = ps.find_nearest(ffs);

#if TEST_USE_ALT_FNN
		auto person_alt = ps.find_nearest_alt(ffs);
		if (person != person_alt)
		{
			std::cerr << "find_nearest_alt invalid result!!!" << std::endl;
		}
#endif

		if (person)
		{
			found_persons.insert(person);
		}
	}

	return found_persons;
}

boost::shared_ptr<FaceInception::CascadeCNN>  FrameFeatures::detector;
boost::shared_ptr<CaffeDetector> FrameFeatures::recognizer;
cv::Mat FrameFeatures::target_mat = cv::Mat(224,224,CV_32FC1);

void FrameFeatures::initialize()
{
#ifndef CPU_ONLY
    int gpu_id = 0;
#else
    int gpu_id = -1;
#endif
	detector.reset(new FaceInception::CascadeCNN("model/det1-memory.prototxt", "model/det1.caffemodel",
									"model/det1-memory-stitch.prototxt", "model/det1.caffemodel",
									"model/det2-memory.prototxt", "model/det2.caffemodel",
									"model/det3-memory.prototxt", "model/det3.caffemodel",
									"model/det4-memory.prototxt", "model/det4.caffemodel",
                    				gpu_id));

	recognizer.reset(new CaffeDetector("model/VGG_FACE_deploy4096_L2.prototxt", "model/VGG_FACE_4096.caffemodel",gpu_id));
}
