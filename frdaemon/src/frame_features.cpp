#include "frame_features.hpp"

#include <random>

std::vector<float> generate_features_for_sample(cv::Mat const & sample)
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

    for(int i=0 ; i< result.size(); i++)
    {

        std::chrono::time_point<std::chrono::system_clock> p3 = std::chrono::system_clock::now();

        faceTransformRegard(image,face,src_points[i],result[i].first,FrameFeatures::target_mat);

        std::vector<float> current_features(FrameFeatures::recognizer->Detect(face));

    }

	return{ SOLUTION_VERSION, 1, 1, 1, 1 };
}

FrameFeatures::FrameFeatures()
{

}

void FrameFeatures::generate_features(cv::Mat const & m)
{
	// 2do: generate frame features from image
		// 2do: generate features for sample image
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
}

bool FrameFeatures::contains_person(std::list<std::vector<float>> const & person_features)
{
	// 2do: check if person is contained on frame

	static std::default_random_engine rng(std::random_device{}());

	return std::uniform_real_distribution<float>(0,100)(rng) < 2.0;

}

boost::shared_ptr<FaceInception::CascadeCNN>  FrameFeatures::detector;
boost::shared_ptr<CaffeDetector> FrameFeatures::recognizer;
cv::Mat FrameFeatures::target_mat = cv::Mat(224,224,CV_32FC1);

void FrameFeatures::initialize()
{
	int gpu_id = -1;
	detector.reset(new FaceInception::CascadeCNN("model/det1-memory.prototxt", "model/det1.caffemodel",
									"model/det1-memory-stitch.prototxt", "model/det1.caffemodel",
									"model/det2-memory.prototxt", "model/det2.caffemodel",
									"model/det3-memory.prototxt", "model/det3.caffemodel",
									"model/det4-memory.prototxt", "model/det4.caffemodel",
                    				gpu_id));

	recognizer.reset(new CaffeDetector("model/VGG_FACE_deploy4096_L2.prototxt", "model/VGG_FACE_4096.caffemodel",gpu_id));
}