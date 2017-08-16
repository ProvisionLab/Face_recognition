#pragma once
#include "cluster.h"
#include "point.h"
#include "convolution_detector.h"
#include <vector>
#include <iostream>

void cardsToPoins(std::vector<Card>& split_hand, std::vector<Point>& points);

void pointsToCards(std::vector<Point>& points, std::vector<Card>& split_hand);

class KMeans
{
private:
	int K; // number of clusters
	int total_values, total_points, max_iterations;
	std::vector<Cluster> clusters;

	// return ID of nearest center (uses euclidean distance)
	int getIDNearestCenter(Point point);

public:
	KMeans(int K, std::vector<Card>& split_hand, int max_iterations);

	void run(std::vector<Point> & points, cv::Size target);
};
