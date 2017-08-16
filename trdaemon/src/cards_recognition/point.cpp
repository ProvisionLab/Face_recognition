#include "point.h"


Point::Point(int id_point, std::vector<double>& values, std::string name = "")
{
	this->id_point = id_point;
	total_values = values.size();

	for (int i = 0; i < total_values; i++)
		this->values.push_back(values[i]);

	this->name = name;
	id_cluster = -1;
}

