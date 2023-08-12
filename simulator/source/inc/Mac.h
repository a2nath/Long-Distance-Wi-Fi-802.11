#pragma once
#include <string>
#include "Phy.h"

/* stores state of the link and msc index */
struct MCSDataManager
{
	struct link_datapair
	{
		uint mcs;
		bool dead;
	};

	umap<station_number, link_datapair> link;

	void add(station_number station, uint _mcs, bool _dead)
	{
		link[station] = { _mcs, _dead };
	}
	void setdead(station_number station)
	{
		link.at(station).dead = true;
	}
	const link_datapair at(station_number station) const
	{
		return link.at(station);
	}
	umap<station_number, mcs_index> mcss()
	{
		umap<station_number, mcs_index> ret;
		for (auto& m : link)
			ret[m.first] = m.second.mcs;
		return ret;
	}
	bool isLinked(station_number station)
	{
		return link.find(station) != link.end();
	}
};

class MacLayer
{
private:
	MCSDataManager lnkmodes;
	PhyAdapter* phylayer;
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
		for (auto& dest_id : Global::connections.dest(self_ID))
		{
			auto rx_signal_power = antenna->getChannel()->get_H_factor(dest_id) + Global::txpowers[self_ID];
			auto noise = antenna->getChannel()->get_effective_noise_dBm();
			double SNR = ((int)((rx_signal_power - noise) * (double)100.0)) / (double)100.0;

			for (int mcs = max_mcs_count - 1; mcs >= 0; --mcs)
			{
				auto per = get_per(mcs2name.at(mcs), SNR);
				if (per <= .01)
				{
					lnkmodes.add(dest_id, mcs, false);
					break;
				}
			}

			if (!lnkmodes.isLinked(dest_id))
			{
				lnkmodes.add(dest_id, 0, true);
			}
		}
		phylayer->get_PhyRate().update(lnkmodes.mcss());
	}

	inline void set_to_dead(station_number station)
	{
		lnkmodes.setdead(station);
	}
	const MCSDataManager& getmap() const
	{
		return lnkmodes;
	}

	unsigned mcs_map(station_number station)
	{
		return lnkmodes.at(station).mcs;
	}

	PhyAdapter* getPhyLayer()
	{
		return phylayer;
	}

	uint getMacID()
	{
		return self_ID;
	}
};
