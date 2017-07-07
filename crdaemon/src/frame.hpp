#pragma once

#include <list>
#include <vector>
#include <set>
#include <memory>
#include <functional>

#include <opencv2/opencv.hpp>

std::vector<std::string> recognize_on_frame(cv::Mat const & frame);
