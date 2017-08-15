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
	// 2do: convert results to json string
	return {};
}

CardsTable::~CardsTable()
{
	uninit();
}

bool CardsTable::init()
{
	// 2do: initialize network

	return true;
}

void CardsTable::uninit()
{
	// 2do: release network resources

}

CardsTable::Results CardsTable::recognize(cv::Mat const & frame)
{
	Results results;

	for (auto && ca :Crops)
	{
		cv::Mat area = get_area(frame, ca);
	}

	for (auto && ca : Buttons)
	{
		cv::Mat area = get_area(frame, ca);
	}

	return results;
}
