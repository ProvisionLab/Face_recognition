#include "frame.hpp"

#include <random>
#include <memory>

#ifdef _WIN32
namespace alpr
{
	class AlprPlate
	{
	public:

	};

	class AlprPlateResult
	{
	public:

		std::vector<AlprPlate> topNPlates;
	}

	class AlprResults
	{
	public:
		std::vector<AlprPlateResult> plates;
	}

	class Alpr
	{
	public: 
		Alpr(char const *, char const *)
		{
		}

		AlprResults recognize(const char *)
		{
			return{};
		}
	};
}
#else

#include <alpr.h>

#endif

std::unique_ptr<alpr::Alpr>	openalpr;

void init_alpr()
{
	if (!openalpr)
	{
		openalpr.reset(new alpr::Alpr("eu", "./openalpr/runtime_data/config/eu.conf", "./openalpr/runtime_data"));
		std::cerr << "openalpr initialized" << std::endl;
	}
}

std::vector<std::string> recognize_on_frame(cv::Mat const & frame)
{
	if (!openalpr)
	{
		init_alpr();
	}

#ifdef _DEBUG
		alpr::AlprResults results = openalpr->recognize("./20170705_122304.jpg");
	//alpr::AlprResults results = openalpr->recognize("./image10.png");

	//std::cout << "alpr: " << results.plates.size() << " results" << std::endl;

	std::vector<std::string> found;

	for (auto & plate : results.plates)
	{
		//std::cout << "plate: " << plate.topNPlates.size() << " results" << std::endl;

		for (auto & candidate : plate.topNPlates)
		{
			//std::cout << "    - " << candidate.characters << "\t confidence: " << candidate.overall_confidence;
			//std::cout << "\t pattern_match: " << candidate.matches_template << std::endl;

			found.push_back(candidate.characters);
			break;
		}
	}

	return found;
#endif

	static std::mt19937 g_rng(std::random_device{}());

	if (std::uniform_real_distribution<float>(0, 100)(g_rng) < 0.2f)
	{
		return { std::to_string(std::uniform_int_distribution<int>(100000, 999999)(g_rng)) };
	}

	return {};
}
