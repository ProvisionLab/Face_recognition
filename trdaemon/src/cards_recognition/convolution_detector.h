#pragma once
#include <caffe/caffe.hpp>
#include "caffe/layers/memory_data_layer.hpp"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>
#include <string>
#include <chrono>

using namespace caffe;
using std::string;

struct Card
{
	int num;
	int x;
	int y;
	int rank;
	int suit;
	int hand_set;
};

string toSuit(int suit);
string toCard(int card);

class ConvolutionDetector
{
public:
	ConvolutionDetector(std::string binary_path);
	~ConvolutionDetector();
	std::vector<Card> predict(int id);
	bool predict(const cv::Mat& img);
	void WrapInputLayer(const cv::Mat &img, std::vector<cv::Mat> *input_channels, int i);
	void setTresh(double tr);
	void scaleFrame(double sc);
	void loadFrame(const string& img_file);
	void loadFrame(cv::Mat& img);
private:
	void local_nms(std::vector<cv::Rect> &bounding_box, std::vector<float> confidence_);
	void global_nms(std::vector<cv::Rect> &bounding_box_, std::vector<float> confidence_);
	string toCard(int card);
	string toSuit(int card);
	double getCard(cv::Rect roi);
	double getSuit(cv::Rect roi);
	float IoU(cv::Rect rect1, cv::Rect rect2);
	double checkBack(cv::Rect roi);
	void saveSamples(std::vector<cv::Rect>& props);
	double thresh_, threshold_NMS_, thresh2_, thresh_cascade2_ , thresh_rank_ ,thresh_suits_, thresh_binary_;
	double scale_;
	cv::Mat img_;
	int img_h_, img_w_;
	boost::shared_ptr<Net<float> > net_;
	boost::shared_ptr<Net<float> > second_net_;
	boost::shared_ptr<Net<float> > card_net_;
	boost::shared_ptr<Net<float> > suit_net_;
	int counter;

};

