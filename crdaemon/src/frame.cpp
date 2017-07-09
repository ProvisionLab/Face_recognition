#include "frame.hpp"

#include <random>

#include "plates_recognition/plates_recognition.hpp"

std::vector<std::string> recognize_on_frame(cv::Mat const & frame)
{
	// static std::mt19937 g_rng(std::random_device{}());

	// if (std::uniform_real_distribution<float>(0, 100)(g_rng) < 0.2f)
	// {
	// 	return { std::to_string(std::uniform_int_distribution<int>(100000, 999999)(g_rng)) };
	// }
	cv::Mat img = frame.clone();
	std::cout<<"recogn on frame"<<std::endl;
	std::vector<std::string> res = plate_recognition(img);

	return res;
}
