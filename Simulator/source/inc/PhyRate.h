#pragma once
#include "timers.h"
#include <unordered_map>
#include <math.h>

class PhyRate
{
	umap<station_number, station_name> phyname;
	umap<station_number, data_rate> phyrate;

public:
	PhyRate() {}
	double getSER(mcs_index mcs, float SNR)
	{
		return get_per(mcs, SNR);
	}
	void update(umap<station_number, mcs_index> link_map)
	{
		for (auto& entry : link_map)
		{
			phyrate[entry.first] = mcs2thru(entry.second);
			phyname[entry.first] = Global::mcs_vs_modname.at(entry.second);
		}
	}
	string getRateName(station_number dest_station)
	{
		return phyname.at(dest_station);
	}
	double getRateSpeed(station_number dest_station)
	{
		return phyrate.at(dest_station);
	}

};