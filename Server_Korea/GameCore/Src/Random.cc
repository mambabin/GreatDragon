#include "Random.hpp"
#include <cstdlib>

int Random_GetRandomIndex(vector<int> chances)
{
	int total = 0;
	for(size_t i = 0; i < chances.size(); i++)
	{
		total += chances[i];
	}
	int randomNum = random() % total;

	int count = 0;
	for(size_t i = 0; i < chances.size(); i++)
	{
		count += chances[i];
		if(count > randomNum)
		{
			return i;
		}
	}
	return 0;//should never reach here
}

