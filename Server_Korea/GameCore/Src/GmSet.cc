#include "GmSet.hpp"
#include "Config.hpp"
#include "Time.hpp"
#include <sys/types.h>
#include <vector>
#include <fstream>
#include <cassert>
#include <string>

using namespace std;

static struct{
	AllGmSet allGmSet;
}package;

void GmSet_LoadData()
{
	string src = Config_DataPath() + string("/GmSet.bytes");
	fstream in(src.c_str(), ios_base::in | ios_base::binary);
	if (in.fail()) {
		DEBUG_LOGERROR("Failed to open %s", src.c_str());
		exit(EXIT_FAILURE);
	}

	if (!package.allGmSet.ParseFromIstream(&in)) {
		DEBUG_LOGERROR("Failed to parse %s", src.c_str());
		exit(EXIT_FAILURE);
	}
}
void GmSet_Init() {
	{
		GmSet_LoadData();
	}
	/* out put config to check
	DEBUG_LOGERROR("GmSet");
	for(int i = 0; i < package.allGmSet.gmSet_size(); i++)
	{
		GmSet* pGmSet = package.allGmSet.mutable_gmSet(i);
		DEBUG_LOGERROR("%d	%d	%s	%s	%s	%s",
				pGmSet->setType(),
				pGmSet->arg(0),
				pGmSet->beginDate().c_str(),
				pGmSet->endDate().c_str(),
				pGmSet->beginTime().c_str(),
				pGmSet->endTime().c_str()
				);
	}
	*/
}
void GmSet_Reload()
{
	package.allGmSet.Clear();
	GmSet_LoadData();
}
bool GmSet_InTime(GmSet* pGmSet)
{
	string beginDate = pGmSet->beginDate();
	string endDate = pGmSet->endDate();
	string beginTime = pGmSet->beginTime();
	string endTime = pGmSet->endTime();

	int beginYear = atoi(beginDate.substr(0, 4).c_str());
	int beginMonth = atoi(beginDate.substr(5, 2).c_str());
	int beginDay = atoi(beginDate.substr(8, 2).c_str());

	int endYear = atoi(endDate.substr(0, 4).c_str());
	int endMonth = atoi(endDate.substr(5, 2).c_str());
	int endDay = atoi(endDate.substr(8, 2).c_str());
	
	bool inDay = Time_Interval(
					time(NULL),
					beginYear, beginMonth, beginDay, 0, 0, 0,
					endYear, endMonth, endDay, 0, 0, 0
					);
	
	if(beginTime == "")
	{
		return inDay;
	}
	else
	{
		int beginHour = atoi(beginTime.substr(0, 2).c_str());
		int beginMin = atoi(beginTime.substr(3, 2).c_str());

		int endHour = atoi(endTime.substr(0, 2).c_str());
		int endMin = atoi(endTime.substr(3, 2).c_str());

		bool inTime = Time_Interval(
						time(NULL),
						2015, 4, 1, beginHour, beginMin, 0,
						2015, 4, 1, endHour, endMin, 0
						);

		return inDay & inTime;
	}
}

int GmSet_GetPercent(GmSet_Type setType, int arg1, int arg2)
{
	for(int i = 0; i < package.allGmSet.gmSet_size(); i++)
	{
		GmSet* pGmSet = package.allGmSet.mutable_gmSet(i);
		if(pGmSet->setType() == setType && GmSet_InTime(pGmSet))
		{
			if(setType == GmSet_Type_STRONG)
			{
				return pGmSet->arg(0);
			}
			else if(setType == GmSet_Type_GOODS)
			{
				if((arg1 == pGmSet->arg(1)) && (arg2 == pGmSet->arg(2)))
				{
					return pGmSet->arg(0);
				}
			}
			else
			{
				if(arg1 == pGmSet->arg(1))
				{
					return pGmSet->arg(0);
				}
			}
		}
	}
	return -1;
}

