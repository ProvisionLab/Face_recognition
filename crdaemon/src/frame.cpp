#include "frame.hpp"
#include "plates_recognition/plates_recognition.hpp"
#include <random>
#include <memory>

// #ifdef _WIN32
// namespace alpr
// {
// 	class AlprPlate
// 	{
// 	public:

// 		std::string characters;
// 		double overall_confidence = 0;
// 		int matches_template = 0;

// 		cv::Mat	image;
// 	};

// 	class AlprPlateResult
// 	{
// 	public:

// 		std::vector<AlprPlate> topNPlates;
// 	};

// 	class AlprResults
// 	{
// 	public:
// 		std::vector<AlprPlateResult> plates;
// 	};

// 	class Alpr
// 	{
// 	public: 
// 		Alpr(char const *, char const *)
// 		{
// 		}

// 		bool isLoaded()
// 		{
// 			return true;
// 		}

// 		AlprResults recognize(const char *)
// 		{
// 			return{};
// 		}
// 	};
// }
// #else

// #include <alpr.h>

// #endif

// std::unique_ptr<alpr::Alpr>	openalpr;


// void test_recognize()
// {
// 	std::cout << "try alpr recognize" << std::endl;

// 	//alpr::AlprResults results = openalpr->recognize("./20170705_122304.jpg");
// 	alpr::AlprResults results = openalpr->recognize("./image10.png");

// 	std::cout << "alpr: " << results.plates.size() << " results" << std::endl;

// 	std::vector<std::string> found;

// 	for (auto & plate : results.plates)
// 	{
// 		std::cout << "plate: " << plate.topNPlates.size() << " results" << std::endl;

// 		for (auto & candidate : plate.topNPlates)
// 		{
// 			std::cout << "    - " << candidate.characters << "\t confidence: " << candidate.overall_confidence;
// 			std::cout << "\t pattern_match: " << candidate.matches_template << std::endl;
// 		}
// 	}
// }

// void init_alpr()
// {
// 	if (!openalpr)
// 	{
// //		openalpr.reset(new alpr::Alpr("eu", "./openalpr/runtime_data/config/eu.conf", "./openalpr/runtime_data"));
// 		openalpr.reset(new alpr::Alpr("eu", "./openalpr.conf"));

// 		if (openalpr->isLoaded())
// 		{
// 			std::cerr << "openalpr initialized" << std::endl;
// 		}
// 		else
// 		{
// 			openalpr.reset();
// 			throw std::runtime_error("Error loading OpenALPR");
// 		}

// #ifdef _DEBUG
// 		test_recognize();
// #endif
// 	}
// }

std::vector<std::pair<std::string, cv::Mat>> recognize_on_frame(cv::Mat const & frame)
{
	std::vector<std::pair<std::string, cv::Mat>> found;

#if 0
	if (!openalpr)
	{
		init_alpr();
	}

	//alpr::AlprResults results = openalpr->recognize("./20170705_122304.jpg");
	alpr::AlprResults results = openalpr->recognize("./image10.png");

	for (auto & plate : results.plates)
	{
		for (auto & candidate : plate.topNPlates)
		{
			found.push_back({ candidate.characters, candidate.image });
			break;
		}
	}

	return found;

#else
	try
	{
		PlateRecognizer recognizer;
		found = recognizer.recognize(frame);
	}
	catch(...)
	{
		return found;
	}
#endif

	return found;
}
