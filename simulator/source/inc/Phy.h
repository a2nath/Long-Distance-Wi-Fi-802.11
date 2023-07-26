#pragma once
#include "Indication.h"

class PhyAdapter
{
private:
	uint phyID;
	PhyRate * phyrate;
	Antenna * antenna;
	Indication *indication;

public:
	transciever_mode antenna_state;
	PhyAdapter(const string name, const map<uint, map<uint, string>>& station_names,
		const map<uint, map<uint, double>>& distance, const map<uint, map<uint, double>>& pathloss) : phyID(Global::sta_name_map.at(name)), antenna_state(RX)
	{
		phyrate = new PhyRate();
		antenna = new Antenna(name, station_names, distance, pathloss);
		indication = new Indication(phyID, &antenna_state, antenna->getChannel());
	}
	Antenna* getAntenna()
	{
		return antenna;
	}
	Indication *get_phy_ind()
	{
		return indication;
	}
	PhyRate &get_PhyRate()
	{
		return *phyrate;
	}
	string getPhyName(station_number dest_station)
	{
		return phyrate->getRateName(dest_station);
	}
	double getPhySpeed(station_number dest_station)
	{
		return phyrate->getRateSpeed(dest_station);
	}
	~PhyAdapter()
	{
		delete phyrate;
		delete antenna;
		delete indication;
	}
	void updateID(int id)
	{
		this->phyID = id;
	}
	int getID()
	{
		return phyID;
	}
};