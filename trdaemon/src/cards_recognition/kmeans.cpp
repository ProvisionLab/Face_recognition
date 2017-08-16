#include "kmeans.h"



KMeans::KMeans(int K, std::vector<Card>& split_hand, int max_iterations)
{
	this->K = K;
	this->total_points = split_hand.size();
	this->total_values = 2;
	this->max_iterations = max_iterations;
}

int KMeans::getIDNearestCenter(Point point)
{
	double sum = 0.0, min_dist;
	int id_cluster_center = 0;

	for (int i = 0; i < total_values; i++)
	{
		sum += pow(clusters[0].getCentralValue(i) -
			point.getValue(i), 2.0);
	}

	min_dist = sqrt(sum);

	for (int i = 1; i < K; i++)
	{
		double dist;
		sum = 0.0;

		for (int j = 0; j < total_values; j++)
		{
			sum += pow(clusters[i].getCentralValue(j) -
				point.getValue(j), 2.0);
		}

		dist = sqrt(sum);

		if (dist < min_dist)
		{
			min_dist = dist;
			id_cluster_center = i;
		}
	}

	return id_cluster_center;
}


void KMeans::run(std::vector<Point> & points, cv::Size target)
{
	if (K > total_points)
		return;

	std::vector<int> prohibited_indexes;

	int c1 = 99;
	std::vector<double> c11 = { 0, (double)target.height / 2 };
	int c2 = 100;;
	std::vector<double> c22 = { (double)target.width, (double)target.height / 2 };
	Point p1(c1, c11, "left");
	Point p2(c2, c22, "right");

	Cluster cluster1(0, p1);
	clusters.push_back(cluster1);

	Cluster cluster2(1, p2);
	clusters.push_back(cluster2);

	// choose K distinct values for the centers of the clusters
	/*
	for (int i = 0; i < K; i++)
	{

	while (true)
	{
	int index_point = rand() % total_points;

	if (find(prohibited_indexes.begin(), prohibited_indexes.end(),
	index_point) == prohibited_indexes.end())
	{
	prohibited_indexes.push_back(index_point);
	points[index_point].setCluster(i);
	Cluster cluster(i, points[index_point]);
	clusters.push_back(cluster);
	break;
	}
	}

	}
	*/

	int iter = 1;

	while (true)
	{
		bool done = true;

		// associates each point to the nearest center
		for (int i = 0; i < total_points; i++)
		{
			int id_old_cluster = points[i].getCluster();
			int id_nearest_center = getIDNearestCenter(points[i]);

			if (id_old_cluster != id_nearest_center)
			{
				if (id_old_cluster != -1)
					clusters[id_old_cluster].removePoint(points[i].getID());

				points[i].setCluster(id_nearest_center);
				clusters[id_nearest_center].addPoint(points[i]);
				done = false;
			}
		}

		// recalculating the center of each cluster
		for (int i = 0; i < K; i++)
		{
			for (int j = 0; j < total_values; j++)
			{
				int total_points_cluster = clusters[i].getTotalPoints();
				double sum = 0.0;

				if (total_points_cluster > 0)
				{
					for (int p = 0; p < total_points_cluster; p++)
						sum += clusters[i].getPoint(p).getValue(j);
					clusters[i].setCentralValue(j, sum / total_points_cluster);
				}
			}
		}

		if (done == true || iter >= max_iterations)
		{
#ifdef KMEANS_OUTPUT
			std::cout << "Break in iteration " << iter << "\n\n";
#endif
			break;
		}

		iter++;
	}

	// shows elements of clusters
#ifdef KMEANS_OUTPUT
	for (int i = 0; i < K; i++)
	{
		int total_points_cluster = clusters[i].getTotalPoints();

		std::cout << "Cluster " << clusters[i].getID() + 1 << std::endl;
		for (int j = 0; j < total_points_cluster; j++)
		{
			std::cout << "Point " << clusters[i].getPoint(j).getID() << ": ";
			for (int p = 0; p < total_values; p++)
				std::cout << clusters[i].getPoint(j).getValue(p) << " ";

			//string point_name = clusters[i].getPoint(j).getName();

			//if (point_name != "")
			//	cout <<"-"<<point_name;

			std::cout << std::endl;
		}

		std::cout << "Cluster values: ";

		for (int j = 0; j < total_values; j++)
			std::cout << clusters[i].getCentralValue(j) << " ";

		std::cout << "\n\n";
	}
#endif
}

void cardsToPoins(std::vector<Card>& split_hand, std::vector<Point>& points)
{
	for (int i = 0; i < split_hand.size(); i++)
	{
		std::vector<double> vals;
		vals.push_back(split_hand[i].x);
		vals.push_back(split_hand[i].y);
		Point temp(split_hand[i].num, vals, std::to_string(split_hand[i].num));
		points.push_back(temp);
	}
}

void pointsToCards(std::vector<Point>& points, std::vector<Card>& split_hand)
{
	for (int i = 0; i < points.size(); i++)
	{
		split_hand[i].hand_set = points[i].getCluster();
	}
}