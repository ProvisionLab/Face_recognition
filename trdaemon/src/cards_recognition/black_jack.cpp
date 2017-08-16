#include "black_jack.h"
bool checkMayBeSplit(std::vector<Card> & A)
{
	if (A.size() != 2)
	{
		//std::cout << "Not 2 cards" << std::endl;
		return false;
	}
	if (A[0].rank == A[1].rank)
	{
		//	std::cout << " Card one " << A[0] % 13 << " Card two " << A[1] % 13 << std::endl;
		return true;
	}
	else
	{
		return false;
	}
}
bool checkSplit(std::vector<Card> & A, int width)
{
	double avg_dist = 0.0;
	for (int i = 0; i < A.size(); i++)
	{
		for (int j = 0; j < A.size(); j++)
		{
			if (i != j)

				avg_dist += std::abs(A[i].x - A[j].x);
		}
	}
	avg_dist /= A.size();
	//std::cout << "Avg dist beetwen cards=" << avg_dist <<" Frame width/3="<<width/3<< std::endl;
	return avg_dist < width / 4 ? false : true;
}
