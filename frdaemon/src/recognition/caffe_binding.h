#pragma once

#ifdef CAFFEBINDING_EXPORTS
#define CAFFE_DLL __declspec(dllexport)
#else
#define CAFFE_DLL __declspec(dllimport)
#endif
//#define CPU_ONLY
#define USE_OPENCV
#include <opencv2/opencv.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace std
{
	template<typename T, typename... Args>
	std::unique_ptr<T> make_unique1(Args&&... args) {
	    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
	}
}

namespace caffe {
  struct DataBlob {
    const float* data;
    std::vector<int> size;
    std::string name;
  };
  class CaffeBinding {
  public:
    CaffeBinding();
    int AddNet(std::string prototxt_path, std::string weights_path, int gpu_id = -1);
    std::unordered_map<std::string, DataBlob> Forward(int net_id);
    std::unordered_map<std::string, DataBlob> Forward(std::vector<cv::Mat>&& input_image, int net_id);
    std::unordered_map<std::string, DataBlob> Forward(std::vector<cv::Mat>& input_image, int net_id) {
      return Forward(std::move(input_image), net_id);
    }
    void SetMemoryDataLayer(std::string layer_name, std::vector<cv::Mat>&& input_image, int net_id);
    void SetMemoryDataLayer(std::string layer_name, std::vector<cv::Mat>& input_image, int net_id) {
      SetMemoryDataLayer(layer_name, std::move(input_image), net_id);
    }
    void SetDevice(int gpu_id);
    ~CaffeBinding();
  };
}
