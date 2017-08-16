#include "cluster.h"



Cluster::Cluster(int id_cluster, Point point)
{
	this->id_cluster = id_cluster;

	int total_values = point.getTotalValues();

	for (int i = 0; i < total_values; i++)
		central_values.push_back(point.getValue(i));

	points.push_back(point);
}
bool Cluster::removePoint(int id_point)
{
	int total_points = points.size();

	for (int i = 0; i < total_points; i++)
	{
		if (points[i].getID() == id_point)
		{
			points.erase(points.begin() + i);
			return true;
		}
	}
	return false;
}

