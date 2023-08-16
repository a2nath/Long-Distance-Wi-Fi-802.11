#pragma once
#include "common.h"
#include "WirelessChannel.h"

class Antenna
{
private:
	uint id;
	bool rx_sig_energy;
	vector<double> gains;
	vector<double> directions;
	double antenna_power;
	WirelessChannel * channel;
	umap <float,float> polarinfo;
public:
	Antenna(string name, const map<uint, map<uint, string>>& station_names,
		const map<uint, map<uint, double>>& distance, const map<uint, map<uint, double>>& pathloss)
		: id(Global::sta_name_map[name]), antenna_power(Global::txpowers[id])
	{
		// bogus data
		polarinfo.insert(pair<float,float>(1, 1.0));
		polarinfo.insert(pair<float, float>(1, 1.0));
		polarinfo.insert(pair<float, float>(1, 1.0));
		polarinfo.insert(pair<float, float>(1, 1.0));

		for (auto it = polarinfo.begin(); it != polarinfo.end(); it++)
		{
			directions.push_back(it->first);
			gains.push_back(it->second);
		}
		channel = new WirelessChannel(name, polarinfo, station_names, distance, pathloss);
	}
	~Antenna()
	{
		delete channel;
	}
	bool is_energized()
	{
		return rx_sig_energy;
	}
	void set_energized(bool state)
	{
		rx_sig_energy = state;
	}
	WirelessChannel* getChannel()
	{
		return channel;
	}
	umap<float, float> getProperties()
	{
		return polarinfo;
	}
	//in dBm
	double getPower()
	{
		return antenna_power;
	}
	uint getID()
	{
		return id;
	}
	double getGain(double angle)
	{
		size_t pos = find(directions.begin(), directions.end(), angle) - directions.begin();
		return pos < directions.size() ? gains[pos] : -1.0;
	}
	double getPowerPlusGain(double angle)
	{
		return getGain(angle) + antenna_power;
	}
};