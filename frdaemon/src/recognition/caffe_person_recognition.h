

#ifndef CAFFE_PERSON_DETECTOR_H
#define CAFFE_PERSON_DETECTOR_H

#define CPU_ONLY
#define USE_OPENCV
#include <caffe/caffe.hpp>
#include <caffe/layers/memory_data_layer.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <string>
#include <vector>

typedef cv::Rect_<double> Rect2d;

using namespace caffe;
using std::string;


class CaffeDetector {
public:
    CaffeDetector(const std::string prototxt_path, const std::string model_path, const int gpuid);
    std::vector<float> Detect(const cv::Mat& face);
    void setOutput(const int output);
    void setInput(const cv::Size target);

    ~CaffeDetector();
private:
    boost::shared_ptr<Net<float> > net_;
    int number_of_features_;
    cv::Size target_;
};



#endif //CAFFE_PERSON_DETECTOR_H
