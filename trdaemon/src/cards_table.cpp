#include "cards_table.hpp"

#include "SimpleJSON/json.hpp"


CardsTable::CropArea area_from_json(json::JSON & ja)
{
	if (ja.JSONType() != json::JSON::Class::Object)
		throw std::invalid_argument(__FUNCTION__);

	CardsTable::CropArea area;

	area.AreaNumber = ja["AreaNumber"].ToInt();
	area.Left = (float)ja["Left"].ToFloat();
	area.Top = (float)ja["Top"].ToFloat();
	area.Right = (float)ja["Right"].ToFloat();
	area.Bottom = (float)ja["Bottom"].ToFloat();

	return area;
}


bool CardsTable::set_config(std::string const & jsonConfig)
{
	Crops.clear();
	Buttons.clear();

	try
	{
		auto obj = json::JSON::Load(jsonConfig);

		if (obj.hasKey("TableNumber"))
		{
			TableNumber = obj["TableNumber"].ToInt();
		}

		if (obj.hasKey("SourceType"))
		{
			SourceType = obj["SourceType"].ToInt();
		}

		if (obj.hasKey("Game"))
		{
			Game = obj["Game"].ToString();
			if (Game == "black-jack")
				bj = true;
		}

		if (obj.hasKey("Crops"))
		{
			auto & arr = obj["Crops"];

			if (arr.JSONType() == json::JSON::Class::Array)
			{
				for (auto &ja : arr.ArrayRange())
				{
					Crops.push_back(area_from_json(ja));
				}
			}
		}

		if (obj.hasKey("Buttons"))
		{
			auto & arr = obj["Buttons"];

			if (arr.JSONType() == json::JSON::Class::Array)
			{
				for (auto &ja : arr.ArrayRange())
				{
					Buttons.push_back(area_from_json(ja));
				}
			}
		}
	}
	catch (...)
	{
		return false;
	}

	return !(Crops.empty() && Buttons.empty());
}

cv::Rect CardsTable::CropArea::get_rect(cv::Size img_size) const
{
	cv::Rect r;
	r.x = int(Left * img_size.width);
	int r_r = int(Right * img_size.width);

	r.width = img_size.width - r.x - r_r;

	r.y = int(Top * img_size.height);
	int r_b = int(Bottom * img_size.height);

	r.height = img_size.height - r.y - r_b;

	return r;
}

cv::Mat CardsTable::get_area(cv::Mat const & frame, CropArea const & area) const
{
	return frame(area.get_rect(frame.size()));
}

std::string CardsTable::Results::to_json() const
{
	auto obj = json::Object();
	std::string string_result;
	string_result = std::to_string(cards.first) + "=";
	for (auto & card : cards.second)
	{
		string_result += std::to_string(card.num) + "," + std::to_string(card.hand_set) + ";";
	}

	obj["Data"] = string_result;

	std::string json = obj.dump();
	// 2do: convert results to json string
	return json;
}

CardsTable::~CardsTable()
{
	uninit();
}

bool CardsTable::init()
{
	detector.reset(new ConvolutionDetector(".\\"));
	// 2do: initialize network

	return true;
}

void CardsTable::uninit()
{
	// 2do: release network resources
	detector.release();
}

std::vector<CardsTable::Results> CardsTable::recognize(cv::Mat const & frame)
{
	current_results.clear();
	for (auto && ca : Crops)
	{
		Results results;
		
		cv::Mat area = get_area(frame, ca);
		
		detector->loadFrame(area);
		std::vector<Card> detection_results = detector->predict(ca.AreaNumber);

		
		for (int i = 0; i < detection_results.size(); i++)
		{
			cv::Rect r(detection_results[i].x, detection_results[i].y, 57, 57);
			cv::rectangle(area, r, cv::Scalar(255, 0, 0));
			string s_card = toCard(detection_results[i].rank);
			string s_suit = toSuit(detection_results[i].suit);
			cv::putText(area, s_card, cv::Point(r.x, r.y + r.height / 2), CV_FONT_HERSHEY_COMPLEX_SMALL, 0.8,
				cv::Scalar(0, 0, 0), 1, CV_AA);
			cv::putText(area, s_suit, cv::Point(r.x + r.width / 2, r.y + r.height / 2), CV_FONT_HERSHEY_COMPLEX_SMALL, 0.8,
				cv::Scalar(0, 0, 0), 1, CV_AA);
			
		}
		if (bj)
		{
			processBlackJack(detection_results, area);
		}

		results.cards = std::make_pair(ca.AreaNumber, detection_results);

		current_results.push_back(results);
		cv::imshow(std::to_string(ca.AreaNumber), area);
		cv::waitKey(1);
	}
	if (Game == "baccarat")
	{
		
		for (auto && ca : Buttons)
		{
			cv::Mat area = get_area(frame, ca);
			auto it = std::find_if(current_results.begin(), current_results.end(),
				[id = ca.AreaNumber](Results const & r) {
				return r.cards.first == id;
			} );
			processBaccarat(*it, area, ca.AreaNumber);
		}
	}
	
	return current_results;
}

void CardsTable::processBlackJack(std::vector<Card> & detection_results, cv::Mat & src)
{
	if (may_be_split)
	{
		if (split)
		{
			std::vector<Point> points;
			cardsToPoins(detection_results, points);
			int k = 2;
			KMeans clustering(k, detection_results, 100);
			clustering.run(points, src.size());
			pointsToCards(points, detection_results);
		}
		else
		{
			split = checkSplit(detection_results, src.cols);
			may_be_split = split ? true : false;
		}

	}
	else
	{
		may_be_split = checkMayBeSplit(detection_results);
	}
}


void CardsTable::processBaccarat(Results & detection_results, cv::Mat & src, int id)
{

	std::pair<cv::Point2f, bool> answer = gotChip(src, id, hasChip, steps);
	if (answer.second)
	{
		int bet = checkBet(src.rows, answer.first);
		Card button;
		button.num = 53;
		button.hand_set = bet;
		detection_results.cards.second.push_back(button);
	}

#ifdef DRAW_CHIPS
	std::string chip_frame_namee = "chips" + std::to_string(id);
	cv::imshow(chip_frame_namee, src);
	cv::waitKey(10);
#endif

}
