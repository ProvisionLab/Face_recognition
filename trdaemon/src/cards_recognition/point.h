#pragma once
#include <string>
#include <vector>
class Point
{
private:
	int id_point, id_cluster;
	std::vector<double> values;
	int total_values;
	std::string name;

public:
	Point(int id_point, std::vector<double>& values, std::string name);

	int getID()
	{
		return id_point;
	}

	void setCluster(int id_cluster)
	{
		this->id_cluster = id_cluster;
	}

	int getCluster()
	{
		return id_cluster;
	}

	double getValue(int index)
	{
		return values[index];
	}

	int getTotalValues()
	{
		return total_values;
	}

	void addValue(double value)
	{
		values.push_back(value);
	}

	std::string getName()
	{
		return name;
	}
};
