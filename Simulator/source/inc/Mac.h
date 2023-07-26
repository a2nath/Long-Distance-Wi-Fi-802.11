#pragma once
#include <string>
#include "Phy.h"

struct McsManager
{
	struct lpair { bool dead; uint mcs; };
	umap<station_number, lpair> link;
	uint& operator[](station_number station) { link[station].dead = false; return link[station].mcs; }
	bool& operator()(station_number station) { return link.at(station).dead; }
	lpair at(station_number station) { return link.at(station); }
	umap<station_number, mcs_index> mcss() { umap<station_number, mcs_index> ret; for (auto m : link) ret[m.first] = m.second.mcs; return ret; }
	bool dead(station_number station, bool isdead) { return link.at(station).dead = isdead; }
	bool find(station_number station) { return link.find(station) != link.end(); }
};

class MacLayer
{
private:
	McsManager lnkmodes;
	PhyAdapter * phylayer;
	station_number self_ID;
public:
	MacLayer(const map<station_number, map<station_number, station_name>>& station_names,
		const map<station_number, map<station_number, double>>& distance,
		const map<station_number, map<station_number, double>>& pathloss, station_name name)
		: self_ID(Global::sta_name_map[name])
	{
		auto& mcs2name = Global::mcs_vs_modname;
		phylayer = new PhyAdapter(name, station_names, distance, pathloss);

		/* Decide MCS for each link */
		auto antenna = phylayer->getAntenna();
		for (auto &dest_id : Global::connections.dest(self_ID))
		{
			double per;
			auto rx_signal_power = antenna->getChannel()->get_H_factor(dest_id) + Global::txpowers[self_ID];
			auto noise = antenna->getChannel()->get_effective_noise_dBm();
			double SNR = ((int)((rx_signal_power - noise) * (double)100.0)) / (double)100.0;

			for (int mcs = max_mcs_count - 1; mcs > -1; --mcs)
			{
				auto per = get_per(mcs2name.at(mcs), SNR);
				if (per <= .01)
				{
					lnkmodes[dest_id] = mcs;
					break;
				}
			}

			if (!lnkmodes.find(dest_id))
			{
				lnkmodes[dest_id] = 0;
				lnkmodes(dest_id) = true;
			}
		}
		phylayer->get_PhyRate().update(lnkmodes.mcss());
	}

	McsManager* getmap()
	{
		return &lnkmodes;
	}

	unsigned mcs_map(station_number station)
	{
		return lnkmodes.at(station).mcs;
	}

	PhyAdapter * getPhyLayer()
	{
		return phylayer;
	}

	uint getMacID()
	{
		return self_ID;
	}
};
