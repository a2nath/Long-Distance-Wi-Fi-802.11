#pragma once
#include <ctime>
#include <iterator>
#include <algorithm>
#include <sstream>
#include <algorithm>
#include "Station.h"

typedef std::shared_ptr<Station> sptrStation;
class Program
{
private:
	map<uint, map<uint, string>> station_names;
	map<uint, map<uint, double>> distance_table;
	map<uint, map<uint, double>> pathloss_table;
	uivector timeline;
	vector<sptrStation> station_list;
	TrafficGenerator TF;
	gcelltable guimap;
	/* Source, Reltime, Dur */
	struct otaobj;
	/* keeps track of the total propagation slots needed for a frame to move bet. stations */
	umap<uint, umap<uint, uint>> proptable;
	vector<vector<sptrFrame>> channel;
	uint system_time;
	uint ENDIT;

	map<uint, float> per_second_thru;
	float total_data;
public:
	Program();
	~Program();
	void done();
	void setup();
	void run();
	void summary();
	gcelltable getgui();
	void update_end_time(int time);
	void print_progress(int time);
	void phy_cca_indication(uint now, uint destination_id);
	bool transmit_frame(uint current_time, std::shared_ptr<Frame> &frame);
};
