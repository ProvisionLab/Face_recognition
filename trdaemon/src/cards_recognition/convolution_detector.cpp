#include "convolution_detector.h"

#ifdef _WIN32
#include "inst_layers.h"
#endif


ConvolutionDetector::ConvolutionDetector(std::string binary_path)
{

	counter = 0;
	threshold_NMS_ = 0.1;
	thresh_ = 0.8;
	thresh_cascade2_ = 0.85;
	thresh_rank_ = 0.85;
	thresh_suits_ = 0.85;
	thresh_binary_ = 195;
	
	std::ifstream			configs("CCNN.cfg");
	std::string cascade1_model, cascade1_trained, cascade2_model, cascade2_trained, cascade3n_model, cascade3n_trained, cascade3s_model, cascade3s_trained;
	if (!configs.is_open())
	{
		std::cout << "Can't open nets config file" << std::endl;
		return;
	}
	configs >> cascade1_model >> cascade1_trained >> cascade2_model >> cascade2_trained >> cascade3n_model >> cascade3n_trained >> cascade3s_model >> cascade3s_trained;
	configs >> threshold_NMS_ >> thresh_ >> thresh_cascade2_ >> thresh_rank_ >> thresh_suits_ >> scale_ >> thresh_binary_;
	configs.close();

	std::cout << "Thresh NMS" << threshold_NMS_ << std::endl;
	std::cout << "thresh_=" << thresh_ << std::endl;
	std::cout << "thresh_cascade2_=" << thresh_cascade2_ << std::endl;
	std::cout << "thresh_rank_=" << thresh_rank_ << std::endl;
	std::cout << "thresh_suits_=" << thresh_suits_ << std::endl;
	std::cout << "scale_=" << scale_ << std::endl;
	std::cout << "thresh_binary_=" << thresh_binary_ << std::endl;
	setTresh(thresh_);
#ifndef CPU_ONLY
	Caffe::set_mode(Caffe::GPU);
	Caffe::SetDevice(0);
#else
	Caffe::set_mode(Caffe::CPU);
#endif
	FLAGS_minloglevel = google::GLOG_FATAL;

	net_.reset(new Net<float>(binary_path + "caffe_nets\\cascade1\\" + cascade1_model, TEST));
	net_->CopyTrainedLayersFrom(binary_path + "caffe_nets\\cascade1\\" + cascade1_trained);

	second_net_.reset(new Net<float>(binary_path + "caffe_nets\\cascade2\\" + cascade2_model, TEST));
	second_net_->CopyTrainedLayersFrom(binary_path + "caffe_nets\\cascade2\\" + cascade2_trained);

	card_net_.reset(new Net<float>(binary_path + "caffe_nets\\ranks\\" + cascade3n_model, TEST));
	card_net_->CopyTrainedLayersFrom(binary_path + "caffe_nets\\ranks\\" + cascade3n_trained);

	suit_net_.reset(new Net<float>(binary_path + "caffe_nets\\suits\\" + cascade3s_model, TEST));
	suit_net_->CopyTrainedLayersFrom(binary_path + "caffe_nets\\suits\\" + cascade3s_trained);

	std::cout << "Conv detector created" << std::endl;

}

std::vector<Card> ConvolutionDetector::predict(int id)
{
	//loadFrame("D:\work\cards_poker\poker_caffe_windows\x64\Release\temp.png");
	std::string frame_name = std::to_string(id) + " thresh_=" + std::to_string(thresh_) + " scale=" + std::to_string(scale_);
	vector<cv::Mat> patches;
	//cv::imwrite("temp.png", img_);
	//  patches.push_back(img_);
#ifdef CAST_TIME
	std::chrono::steady_clock::time_point cas1_start = std::chrono::steady_clock::now();
#endif
	cv::Mat showimg = img_.clone();

	//  cv::resize(showimg, showimg,cv::Size(showimg.cols*0.21, showimg.rows*0.21));
	cv::Mat drawimg = showimg.clone();
	cv::cvtColor(showimg, showimg, CV_BGR2GRAY);

	cv::threshold(showimg, showimg, thresh_binary_, 255, CV_THRESH_OTSU & CV_THRESH_BINARY);

	//cv::imshow("binaryCas1", showimg);
	//cv::waitKey(10);
	patches.push_back(showimg);
	float loss = 0.0;
	boost::shared_ptr<MemoryDataLayer<float> > mdl;

	mdl = boost::static_pointer_cast<MemoryDataLayer<float> >(net_->layer_by_name("data"));

	vector<int> labels(patches.size());
	mdl->AddMatVector(patches, labels);
	// net_->Reshape();
	
	
	const vector<Blob<float>*> & results = net_->Forward(&loss);


	float *output = results[1]->mutable_cpu_data();


	/*
	std::cout << "results[1]->num(0) = " << results[1]->num() << std::endl;
	std::cout << "results[1]->count(1) = " << results[1]->channels() << std::endl;
	std::cout << "results[1]->count(2) = " << results[1]->width() << std::endl;
	std::cout << "results[1]->count(3) = " << results[1]->height() << std::endl;
	std::cout << "results[1]->count() = " << results[1]->count() << std::endl;
	*/
	int w = results[1]->width();
	int h = results[1]->height();
	int c = results[1]->count() / 2;

	int stride_x = std::ceil(img_w_ / w);
	int stride_y = std::ceil(img_h_ / h);
	int cellSize = 57;

//	std::cout << "tick " << std::to_string(id) <<std::endl;
	std::vector<cv::Rect> proposals, proposals_cascade2;
	std::vector<float> confidence, confidence_cascade2;
	int x, y;
	for (int i = 0; i < c; i++)
	{

		x = (i % w);//* img_w_ / w;
		y = (i / w);//* img_h_ / h;

		cv::Rect roi;
		roi.x = x*stride_x;
		roi.y = y*stride_y;
		roi.width = cellSize;
		roi.height = cellSize;

		if (output[c + i] > thresh_)
		{
			proposals.push_back(roi);
			confidence.push_back(output[c + i]);

			cv::rectangle(drawimg, roi, cv::Scalar(0, 255, 0), 1);
			//printf("Card=%d detected \n", card);
			// printf("##Probability of card %d %d is %.3f    > thresh\n", x, y, output[c + i]);
		}


	}
#ifdef CAST_TIME
	std::chrono::steady_clock::time_point cas1_end = std::chrono::steady_clock::now();

	std::cout << "Cas1 time = " << std::chrono::duration_cast<std::chrono::milliseconds>(cas1_end - cas1_start).count() << std::endl;
	//  saveSamples(proposals);
	std::chrono::steady_clock::time_point lmnm_start = std::chrono::steady_clock::now();
	std::cout<<"Before NMS1>>"<<proposals.size()<<std::endl;
#endif
	local_nms(proposals, confidence);
#ifdef CAST_TIME
    std::cout<<"After NMS1>>"<<proposals.size()<<std::endl;
	std::chrono::steady_clock::time_point lmnm_end = std::chrono::steady_clock::now();

	std::cout << "Local NMS time = " << std::chrono::duration_cast<std::chrono::milliseconds>(lmnm_end - lmnm_start).count() << std::endl;
#endif
	std::vector<cv::Rect> forsave;

	int card, suit;


	//    cv::imshow(frame_name, drawimg);
	//    cv::waitKey(10);
#ifdef CAST_TIME
	std::chrono::steady_clock::time_point cas2_start = std::chrono::steady_clock::now();
#endif
	std::vector<Card> result;
	for (int i = 0; i<proposals.size(); i++)
	{
		cv::rectangle(drawimg, proposals[i], cv::Scalar(0, 255, 255), 1);
		try
		{
			cv::Rect r = proposals[i];


			//        const int RESIZE = 7;
			//        r.x -= RESIZE;
			//        r.y -= RESIZE;
			//        r.width += RESIZE * 2;
			//        r.height += RESIZE * 2;

			double prop_cascade2 = checkBack(r);

			if (prop_cascade2 > thresh_cascade2_)
			{
				proposals_cascade2.push_back(r);
				confidence_cascade2.push_back(prop_cascade2);
				//cv::rectangle(drawimg, r, cv::Scalar(255,125,0), 1);

			}
			else
			{
				// cv::rectangle(drawimg, r, cv::Scalar(0,0,255), 1);

			}

			// forsave.push_back(r);
		}
		catch (...)
		{
			
			continue;
		}
	}
	std::chrono::steady_clock::time_point cas2_end = std::chrono::steady_clock::now();

#ifdef CAST_TIME
	std::cout << "Cas2 time = " << std::chrono::duration_cast<std::chrono::milliseconds>(cas2_end - cas2_start).count() << std::endl;
	//  saveSamples(forsave);
	std::chrono::steady_clock::time_point gnms_start = std::chrono::steady_clock::now();
	//  std::cout<<"Before NMS2>>"<<proposals_cascade2.size()<<std::endl;
#endif
	global_nms(proposals_cascade2, confidence_cascade2);

#ifdef CAST_TIME
	std::chrono::steady_clock::time_point gnms_end = std::chrono::steady_clock::now();

	std::cout << "Global NMS time = " << std::chrono::duration_cast<std::chrono::milliseconds>(gnms_end - gnms_start).count() << std::endl;
	//  std::cout<<"After NMS2>>"<<proposals_cascade2.size()<<std::endl;

	//   saveSamples(proposals_cascade2);
	std::chrono::steady_clock::time_point class_start = std::chrono::steady_clock::now();
#endif
	for (int i = 0; i< proposals_cascade2.size(); i++)
	{
		try
		{
			Card temp;
			cv::Rect r = proposals_cascade2[i];
			card = getCard(r);
			suit = getSuit(r);
			temp.num = suit * 13 + card + 1;
			temp.x = r.x ;
			temp.y = r.y;
			temp.rank = card;
			temp.suit = suit;
			temp.hand_set = 0;
			result.push_back(temp);
			//cv::rectangle(drawimg, r, cv::Scalar(255, 0, 0), 1);
		//	string s_card = toCard(card);
		//	string s_suit = toSuit(suit);
		//	cv::putText(drawimg, s_card, cv::Point(r.x, r.y + r.height / 2), CV_FONT_HERSHEY_COMPLEX_SMALL, 0.8,
		//		cv::Scalar(0, 0, 0), 1, CV_AA);
		//	cv::putText(drawimg, s_suit, cv::Point(r.x + r.width / 2, r.y + r.height / 2), CV_FONT_HERSHEY_COMPLEX_SMALL, 0.8,
		//		cv::Scalar(0, 0, 0), 1, CV_AA);
		}
		catch (...)
		{
			continue;
		}
	}
#ifdef CAST_TIME
	std::chrono::steady_clock::time_point class_end = std::chrono::steady_clock::now();

	std::cout << "Classification time = " << std::chrono::duration_cast<std::chrono::milliseconds>(class_end - class_start).count() << std::endl;
#endif
	//if (drawimg.cols > 800 || drawimg.rows > 1200)
	//	cv::resize(drawimg, drawimg, cv::Size(drawimg.cols / 2, drawimg.rows / 2));


	//cv::imshow(frame_name, drawimg);
	//cv::waitKey(10);

	return result;
	// printf("Card %d :%.3f\n",max_pos, max);
}
double ConvolutionDetector::getCard(cv::Rect roi)
{

	// roi.x += 10;
	// roi.x -=3;
	 //roi.y -= 2;
	cv::Mat sample = img_(roi);


	// cv::resize(sample, sample, cv::Size(28,28));
	cv::cvtColor(sample, sample, CV_BGR2GRAY);

	cv::threshold(sample, sample, thresh_binary_, 255, CV_THRESH_OTSU & CV_THRESH_BINARY);


	// cv::Mat sample2;
	// cv::adaptiveThreshold(sample, sample2, 255, CV_ADAPTIVE_THRESH_GAUSSIAN_C, CV_THRESH_BINARY, 11, 2);

	//    int erosion_type = cv::MORPH_RECT;
	//    int erosion_size = 1;
	////    else if( erosion_elem == 1 ){ erosion_type = MORPH_CROSS; }
	////    else if( erosion_elem == 2) { erosion_type = MORPH_ELLIPSE; }

	//    cv::Mat element = cv::getStructuringElement( erosion_type,
	//                                         cv::Size( 2*erosion_size + 1, 2*erosion_size+1 ),
	//                                         cv::Point( erosion_size, erosion_size ) ), erosion_dst;

	//    /// Apply the erosion operation
	//    cv::erode( sample, erosion_dst, element );
	// cv::imshow("erosion_dst", erosion_dst);

	// cv::waitKey(0);

	vector<cv::Mat> patches;
	patches.push_back(sample);

	float loss = 0.0;
	boost::shared_ptr<MemoryDataLayer<float> > mdl;

	mdl = boost::static_pointer_cast<MemoryDataLayer<float> >(card_net_->layer_by_name("data"));

	vector<int> labels(patches.size());
	mdl->AddMatVector(patches, labels);
	// net_->Reshape();

	const vector<Blob<float>*> & results = card_net_->Forward(&loss);



	float *output = results[1]->mutable_cpu_data();

	//    std::cout << sample.rows<< " "<<sample.cols<<std::endl;
	//    std::cout << "results[1]->num(0) = " << results[1]->num() << std::endl;
	//    std::cout << "results[1]->count(1) = " << results[1]->channels() << std::endl;
	//    std::cout << "results[1]->count(2) = " << results[1]->width() << std::endl;
	//    std::cout << "results[1]->count(3) = " << results[1]->height() << std::endl;
	//    std::cout << "results[1]->count() = " << results[1]->count() << std::endl;


	float max = 0.0;
	int max_pos = 0;

	for (int i = 0; i < 13; i++)
	{
		if (output[i] > max)
		{
			max = output[i];
			max_pos = i;
		}
	}
	// std::cout<<"Rank="<<max_pos<<" Prop="<<max<<std::endl;
	//cv::imshow(std::to_string(max_pos), sample);
	//cv::waitKey(10);
	// cv::waitKey(10);
	if (max > thresh_rank_)
		return max_pos;
	else
		return -1;




}
float ConvolutionDetector::IoU(cv::Rect rect1, cv::Rect rect2)
{
	int x_overlap, y_overlap, intersection, unions;
	x_overlap = std::max(0, std::min((rect1.x + rect1.width), (rect2.x + rect2.width)) - std::max(rect1.x, rect2.x));
	y_overlap = std::max(0, std::min((rect1.y + rect1.height), (rect2.y + rect2.height)) - std::max(rect1.y, rect2.y));
	intersection = x_overlap * y_overlap;
	unions = rect1.width * rect1.height + rect2.width * rect2.height - intersection;
	return float(intersection) / unions;
}

double ConvolutionDetector::getSuit(cv::Rect roi)
{
	// roi.x += 5;

	cv::Mat sample = img_(roi);
	//  sample = cv::imread("/home/greeser/Work/cards_recognition/4class/1/1386.png");
	//  cv::resize(sample, sample, cv::Size(52,52));
	// cv::cvtColor(sample, sample, CV_BGR2GRAY);

	// cv::imshow("suits", sample);
	vector<cv::Mat> patches;
	patches.push_back(sample);

	float loss = 0.0;
	boost::shared_ptr<MemoryDataLayer<float> > mdl;

	mdl = boost::static_pointer_cast<MemoryDataLayer<float> >(suit_net_->layer_by_name("data"));

	vector<int> labels(patches.size());
	mdl->AddMatVector(patches, labels);
	// net_->Reshape();

	const vector<Blob<float>*> & results = suit_net_->Forward(&loss);



	float *output = results[1]->mutable_cpu_data();

	//    std::cout << sample.rows<< " "<<sample.cols<<std::endl;
	//    std::cout << "results[1]->num(0) = " << results[1]->num() << std::endl;
	//    std::cout << "results[1]->count(1) = " << results[1]->channels() << std::endl;
	//    std::cout << "results[1]->count(2) = " << results[1]->width() << std::endl;
	//    std::cout << "results[1]->count(3) = " << results[1]->height() << std::endl;
	//    std::cout << "results[1]->count() = " << results[1]->count() << std::endl;


	float max = 0.0;
	int max_pos = 0;

	for (int i = 0; i < 4; i++)
	{
		//   std::cout<<output[i]<<std::endl;
		if (output[i] > max)
		{
			max = output[i];
			max_pos = i;
		}
	}
	// std::cout<<"Suit="<<max_pos<<" Prop="<<max<<std::endl;

	//  cv::waitKey(0);
	if (max > thresh_suits_)
		return max_pos;
	else
		return -1;
}

double ConvolutionDetector::checkBack(cv::Rect roi)
{
	cv::Mat sample;
	try {
		sample = img_(roi);
		//загрузи картинку здесь с файла
		//тут же на линуксе сохрани один пример картинки
		//cv::imwrite("cas2.png", sample);
	//	cv::imshow("smpl", sample);
	//	cv::waitKey(0);
		//cv::resize(sample, sample, cv::Size(52, 52));
		//sample = cv::imread("/home/greeser/Work/cards_recognition/cascade2-background/samplesPng/85.png");
		//sample = cv::imread("/home/greeser/Work/cards_recognition/suits-dataset/1/0/1.png");
		// cv::cvtColor(sample, sample, cv::COLOR_BGR2GRAY);
	}
	catch (...)
	{
		return 0;
	}

	vector<cv::Mat> patches;
	patches.push_back(sample);

	// cv::imshow("2", sample);
	// cv::waitKey(0);

	float loss = 0.0;
	boost::shared_ptr<MemoryDataLayer<float> > mdl;

	mdl = boost::static_pointer_cast<MemoryDataLayer<float> >(second_net_->layer_by_name("data"));

	vector<int> labels(patches.size());
	mdl->AddMatVector(patches, labels);
	// net_->Reshape();

	const vector<Blob<float>*> & results = second_net_->Forward(&loss);



	//    std::cout << results.size() << std::endl;
	//    std::cout << results[0]->count() << std::endl;
	float *output = results[1]->mutable_cpu_data();

	//    std::cout << sample.rows<< " "<<sample.cols<<std::endl;


	//    std::cout << "================"    << std::endl;
	//    std::cout << "results[0]->num(0) = " << results[0]->num() << std::endl;
	//    std::cout << "results[0]->count(1) = " << results[0]->channels() << std::endl;
	//    std::cout << "results[0]->count(2) = " << results[0]->width() << std::endl;
	//    std::cout << "results[0]->count(3) = " << results[0]->height() << std::endl;
	//    std::cout << "results[0]->count() = " << results[0]->count() << std::endl;

	//std::cout << "================"    << std::endl;

	//    std::cout << "results[1]->num(0) = " << results[1]->num() << std::endl;
	//    std::cout << "results[1]->count(1) = " << results[1]->channels() << std::endl;
	//    std::cout << "results[1]->count(2) = " << results[1]->width() << std::endl;
	//    std::cout << "results[1]->count(3) = " << results[1]->height() << std::endl;
	//    std::cout << "results[1]->count() = " << results[1]->count() << std::endl;

	//    std::cout << "================"    << std::endl;
	//    std::cout << "================"    << std::endl;

	//    float max = 0.0;
	//    int max_pos=0;

	//    for (int i = 0; i < 2; i++)
	//    {
	//        if(output[i] > max)
	//        {
	//            max=output[i];
	//            max_pos=i;
	//        }
	//    }
	//   std::cout<<"Cascade2 prop0="<<output[0]<<std::endl;

	//   // cv::waitKey(0);
	if (output[1] > thresh_cascade2_)
	{
      //  std::cout<<"Cascade2 prop1="<<output[1]<<std::endl;
		return output[1];
	}
	else
		return 0;



}


void ConvolutionDetector::local_nms(std::vector<cv::Rect>& bounding_box_, std::vector<float> confidence_)
{
	std::vector<cv::Rect> cur_rects = bounding_box_;
	std::vector<float> confidence = confidence_;
	float threshold = threshold_NMS_;

	for (int i = 0; i < cur_rects.size(); i++)
	{
		for (int j = i + 1; j < cur_rects.size();)
		{
			if (IoU(cur_rects[i], cur_rects[j]) > threshold)
			{
				//                float a = IoU(cur_rects[i], cur_rects[j]);
				//                if(confidence[i] == confidence[j])
				//                {
				//                    cur_rects.erase(cur_rects.begin() + j);
				//                    confidence.erase(confidence.begin() + j);
				//                }
				int dx = std::abs(cur_rects[i].x - cur_rects[j].x);
				int dy = std::abs(cur_rects[i].y - cur_rects[j].y);
				int d = std::sqrt(dx * dx + dy * dy);
				if (confidence[i] >= confidence[j] && (confidence[j] < 0.995 || d < 7))
				{
					cur_rects.erase(cur_rects.begin() + j);
					confidence.erase(confidence.begin() + j);
				}
				else if (confidence[i] < confidence[j] && (confidence[i] < 0.995 || d < 7))
				{
					cur_rects.erase(cur_rects.begin() + i);
					confidence.erase(confidence.begin() + i);
					i--;
					break;
				}
				else
				{
					j++;
				}
			}
			else
			{
				j++;
			}

		}
	}

	bounding_box_ = cur_rects;
	confidence_ = confidence;
}

void ConvolutionDetector::global_nms(std::vector<cv::Rect> &bounding_box_, std::vector<float> confidence_)
{
	std::vector<cv::Rect> cur_rects = bounding_box_;
	std::vector<float> confidence = confidence_;
	float threshold = threshold_NMS_;

	for (int i = 0; i < cur_rects.size(); i++)
	{
		for (int j = i + 1; j < cur_rects.size();)
		{
			if (IoU(cur_rects[i], cur_rects[j]) > threshold)
			{
				if (confidence[i] >= confidence[j])
				{
					cur_rects.erase(cur_rects.begin() + j);
					confidence.erase(confidence.begin() + j);
				}
				else if (confidence[i] < confidence[j])
				{
					cur_rects.erase(cur_rects.begin() + i);
					confidence.erase(confidence.begin() + i);
					i--;
					break;
				}
				else
				{
					j++;
				}
			}
			else
			{
				j++;
			}

		}
	}

	bounding_box_ = cur_rects;
	confidence_ = confidence;
}

void ConvolutionDetector::setTresh(double tr)
{
	thresh_ = tr;
}

void ConvolutionDetector::scaleFrame(double sc)
{
	scale_ = sc;
	cv::Mat img;
	cv::Size trg;
	trg.height = img_.rows * scale_;
	trg.width = img_.cols * scale_;;
	cv::resize(img_, img, trg);
	img_ = img;
	img_h_ = img_.rows;
	img_w_ = img_.cols;
}

void ConvolutionDetector::loadFrame(const string &img_file)
{
	img_ = cv::imread(img_file, CV_LOAD_IMAGE_COLOR);
	img_h_ = img_.rows;
	img_w_ = img_.cols;
}

void ConvolutionDetector::loadFrame(cv::Mat &img)
{
	img_ = img;
	img_h_ = img_.rows;
	img_w_ = img_.cols;

	if ( scale_ != 1.0)
		scaleFrame(scale_);

	img = img_.clone();
}
ConvolutionDetector::~ConvolutionDetector()
{
}


string ConvolutionDetector::toSuit(int suit)
{
	if (suit == 0)
	{
		return "S";
	}
	if (suit == 1)
	{
		return "H";
	}
	if (suit == 2)
	{
		return "D";
	}
	if (suit == 3)
	{
		return "C";
	}
}

string ConvolutionDetector::toCard(int card)
{
	if (card == -1)
	{
		return "N";
	}
	if (card == 0)
	{
		return "A";
	}
	if (card == 12)
	{
		return "K";
	}
	if (card == 11)
	{
		return "Q";
	}
	if (card == 10)
	{
		return "J";
	}
	if (card > 0 && card < 10)
	{
		card++;
		return std::to_string(card);
	}
}


string toSuit(int suit)
{
	if (suit == -1)
	{
		return "N";
	}
	if (suit == 0)
	{
		return "S";
	}
	if (suit == 1)
	{
		return "H";
	}
	if (suit == 2)
	{
		return "D";
	}
	if (suit == 3)
	{
		return "C";
	}
}

string toCard(int card)
{
	if (card == -1)
	{
		return "N";
	}
	if (card == 0)
	{
		return "A";
	}
	if (card == 12)
	{
		return "K";
	}
	if (card == 11)
	{
		return "Q";
	}
	if (card == 10)
	{
		return "J";
	}
	if (card > 0 && card < 10)
	{
		card++;
		return std::to_string(card);
	}
}