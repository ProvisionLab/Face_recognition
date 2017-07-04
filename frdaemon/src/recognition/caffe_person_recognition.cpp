
#include "caffe_person_recognition.h"

CaffeDetector::CaffeDetector(const std::string prototxt_path, const std::string model_path, const int gpuid=-1)
{
   // std::ifstream net_params(prototxt_path + "/config.cfg");
    //std::string net_prototxt, net_model;
    //net_params >> net_prototxt >> net_model;
    //net_params.close();
    if (gpuid < 0)
	{
		std::cout<<"CPU MODE"<<std::endl;
        Caffe::set_mode(Caffe::CPU);
	}
    else
    {

		std::cout<<"GPU MODE"<<std::endl;
        Caffe::set_mode(Caffe::GPU);
        Caffe::SetDevice(gpuid);
    }
    std::cout<<prototxt_path << std::endl;
    std::cout << model_path << std::endl;
    net_.reset(new Net<float>(prototxt_path, TEST));
    net_->CopyTrainedLayersFrom(model_path);
    number_of_features_ = 4096;
    target_ = cv::Size(224,224);

}
CaffeDetector::~CaffeDetector()
{

}


std::vector<float> CaffeDetector::Detect(const cv::Mat& card)
{
    cv::Mat normalized(target_, CV_32FC1);
    cv::resize(card, normalized, target_);
    vector<cv::Mat> patches;
    patches.push_back(normalized);

    float loss=0.0;
    boost::shared_ptr<MemoryDataLayer<float> > types_mdl;

    types_mdl = boost::static_pointer_cast<MemoryDataLayer<float> >(net_->layer_by_name("data"));

    vector<int> types_labels(patches.size());
    types_mdl->AddMatVector(patches, types_labels);
   // std::cout<<"flag1"<<std::endl;
    const vector<Blob<float>*>& types_results = net_->Forward(&loss);
  //   std::cout<<"flag2"<<std::endl;

    const float *types_prob = types_results[1]->mutable_cpu_data();
  //   std::cout<<"flag3"<<std::endl;
    std::vector<float> probs(types_prob, types_prob + number_of_features_);
  //  std::cout<<"flag4"<<std::endl;
    int type=0;
    float max=types_prob[0];

    for(int i=0; i< number_of_features_; ++i)
    {
        if (types_prob[i] > max) {
            max = types_prob[i];
            type = i;
        }
      //  std::cout<<probs[i]<<" ";

    }
   // std::cout<<std::endl;
   // std::wstring res=std::to_wstring(type);

    return probs;
}

void CaffeDetector::setOutput(const int output)
{
    number_of_features_ = output;
}

void CaffeDetector::setInput(const cv::Size target)
{
    target_ = target;
}

