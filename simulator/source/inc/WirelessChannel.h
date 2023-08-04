#pragma once
#include "common.h"
#include "Location.h"

using namespace std;

class WirelessChannel
{
private:

	/*noise characteristics*/
	float thermal_noise;
	float EI_Noise;

	uint self_id;
	float bandwidth;
	umap<station_number, double> hmatrixMap, distanceMap;
	umap<station_number, prop_del_us> propagationMap;

public:
	WirelessChannel(const string name, umap<float, float>& antenna_prop, const map<uint, map<uint, string>>& station_names,
		const map<uint, map<uint, double>>& distance, const map<uint, map<uint, double>>& pathloss) : self_id(Global::sta_name_map[name])
	{
		auto propfactor = Global::prop_factor;
		auto station_count = Global::station_count;
		auto polar_info = antenna_prop;
		bandwidth = Global::bandwidth;
		thermal_noise = thermal_per_bandwidth + 10.0 * log10(bandwidth);
		EI_Noise = thermal_noise + system_noise_figure;

		double gain1 = name == Global::ap_station ? G_ap : G_cl;
		for (uint sta = 0; sta < station_count; ++sta)
		{
			if (sta == self_id) continue;
			double gain2 = Global::sta_name_map[Global::ap_station] == sta ? G_ap : G_cl;
			distanceMap[sta] = distance.at(self_id).at(sta) * 1e3;

			if (0 < propfactor && propfactor <= 1) //0 - 100% distance scale
			{
				propagationMap[sta] = round(propfactor * s2micro((double)(distanceMap.at(sta) / lightspeed)));
			}
			else
			{
				propagationMap[sta] = 1; // micro-second
			}

			hmatrixMap[sta] =  gain1 + gain2 - pathloss.at(self_id).at(sta);
		}
	}
	// in dB
	float get_effective_noise_dBm()
	{
		return EI_Noise;
	}
	// in Watts
	float get_effective_noise()
	{
		return dBm2W(EI_Noise);
	}
	float getBandwidth()
	{
		return bandwidth;
	}
	//in dB
	double get_H_factor(int station)
	{
		return hmatrixMap.at(station);
	}
	umap<station_number, prop_del_us> getPropgationMap()
	{
		return propagationMap;
	}
	uint getPropDelay(int station)
	{
		return propagationMap.at(station);
	}
	double getDistance(int station)
	{
		return distanceMap.at(station);
	}
};