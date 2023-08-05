#include <typeinfo>
#include <iomanip>
#include <chrono>
#include "../inc/Program.h"

#define AP_MODE
#define str2uint str2num<uint>
#define str2double str2num<double>
#define str2long str2num<unsigned long int>

vector<station_number> inters;
int progress_printed = -1;
uint sliding_window = 0;
uint simultaneous_tx;
uint end_time;
Logs logs;
string path;

void log_throughput(uint now, float &total, vector<sptrStation> &stations, map<uint, float> &thru)
{
	float data_per_sec = total;
	float total_data_this_sec = 0;
	for (auto sta : stations)
	{
		total_data_this_sec += sta->total_data_transferred();
	}
	total = total_data_this_sec;
	thru[now] = total - data_per_sec;
}
Program::Program(unordered_map<string, string>& arglist) : total_data(0)
{
	/* set the input parameters if set by the driving file */
	if (arglist.find("input_dir") != arglist.end())
		input_dir             = arglist.at("input_dir");
	if (arglist.find("output_dir") != arglist.end())
		output_dir            = arglist.at("output_dir");
	if (arglist.find("per_rate_file") != arglist.end())
		per_rate_file         = arglist.at("per_rate_file");
	if (arglist.find("error_rate_file") != arglist.end())
		error_rate_file       = arglist.at("error_rate_file");
	if (arglist.find("mapping_file") != arglist.end())
		in_mapping_file       = arglist.at("mapping_file");
	if (arglist.find("distance_map") != arglist.end())
		in_distance_map       = arglist.at("distance_map");
	if (arglist.find("pathloss_map") != arglist.end())
		in_pathloss_map       = arglist.at("pathloss_map");
	if (arglist.find("simulation_params") != arglist.end())
		in_simulation_params = arglist.at("simulation_params");

	setup();
	debugout("Progress [ ");
	run();
	debugout("]\n");
	summary();
	done();
}
Program::~Program()
{
	for (auto &cellrow : guimap)
	{
		for (uint i = 0; i < cellrow.size(); ++i)
			delete cellrow[i].receiver, cellrow[i].transmitter;
		cellrow.clear();
	}
	guimap.clear();
}

void Program::done()
{
	logs.done(double2str(system_time/1000.0, 3));
}

struct Program::otaobj
{
	struct ota_data { uint source, reltime, dur; };
	vector<ota_data> buff;
	void add(uint source, uint release_time, uint duration)
	{
		buff.push_back(ota_data{ source, release_time, duration });
		if (buff.size() > simultaneous_tx)
			++sliding_window;
	}
	int size() { return buff.size(); }
	ota_data operator[](uint idx) { return buff[idx]; }
	ota_data last() { return buff.back(); }
} otalist;

string create_tstamp() {
	char* rc;
	char timestamp[30];
	time_t rawtime = time(0);
	tm *now = localtime(&rawtime);
	if (rawtime != -1) strftime(timestamp, 30, "%Y-%m-%d_%H%M%S", now);
	return string(timestamp);
}

void Program::setup()
{
	/* Read the station names */
	vector<vector<string>> output;
	vector<vector<string>> sta_names;
	vector<vector<double>> sta_distances;
	vector<vector<double>> sta_pathlosses;

	chdir("../");

	IO::readfile(input_dir + in_mapping_file, output);

	sta_names.resize(output.size(), vector<string>(output.size()));
	sta_distances.resize(sta_names.size(), vector<double>(sta_names.size()));
	sta_pathlosses.resize(sta_names.size(), vector<double>(sta_names.size()));

	for (int i = 0; i < sta_names.size(); ++i)
		for (int j = 0; j < sta_names[i].size(); ++j)
			sta_names[i][j] = output[i][j];

	/* Read the MxM Distance table (km) */
	IO::readfile(input_dir + in_distance_map, output);

	for (int i = 0; i < sta_distances.size(); ++i)
		for (int j = 0; j < sta_distances[i].size(); ++j)
			sta_distances[i][j] = str2double(output[i][j]);

	/* Read the MxM Pathloss table (dB) */
	IO::readfile(input_dir + in_pathloss_map, output);

	for (int i = 0; i < sta_pathlosses.size(); ++i)
		for (int j = 0; j < sta_pathlosses[i].size(); ++j)
			sta_pathlosses[i][j] = str2double(output[i][j]);

	/* Read the input parameters from the INPUT file */
	vector<string> selected_stations;
	vector<string> input_file_contents = IO::readfile(input_dir + in_simulation_params, output, true);

	Global::traffic_type = output[conns][0] == "udp" ? 0 : 1;
	Global::DEBUG_END = str2double(output[debugend][0]) * 1000;
	ENDIT = Global::DEBUG_END;

	for (auto& s : output[antpower])
		Global::txpowers.emplace_back(str2double(s));

	Global::data_pack_size = str2double(output[datbytes][0]);
	Global::produration = str2double(output[progdur][0]) * 1000; // in microseconds
	Global::simduration = str2double(output[simdur][0]) * 1000; // in microseconds
	Global::data_fragments = str2double(output[dsegments][0]);
	Global::frequency = str2double(output[fquency][0]);
	Global::bandwidth = str2double(output[bwidth][0]);
	Global::adapt_int_tout = output[atout][0] == "yes" ? true : false;
	selected_stations = output[stanames];
	selected_stations.emplace_back(Global::ap_station);
	Global::station_count = selected_stations.size();
	Global::prop_factor = str2double(output[pfactor][0]);
	Global::chwindow = str2uint(output[chwdow][0]);
	Global::aCWmax = str2uint(output[acwmax][0]);
	Global::aCWmin = str2uint(output[acwmin][0]);
#ifdef REDUNDANT_RETRIES
	if (!str2uint(output[relim][0])) error_out("Retry number problem");
	Global::dot11ShortRetryLimit = str2uint(output[relim][0]);
#else
	uint re = str2uint(output[relim][0]);
	uint validre = log2(Global::aCWmax) - log2(Global::aCWmin) + 1;
	Global::dot11ShortRetryLimit = validre != re ? validre : re;
#endif

	dout("\n\n>>>>>>>>>>>>>>>>>>>>>>>   RETRY LIMIT: " + num2str(Global::dot11ShortRetryLimit) + "   <<<<<<<<<<<<<<<<<<");
	dout(">>>>>>>>>>>>>>>>>>>>>>>   CWMAX: " + num2str(Global::aCWmax) + "      <<<<<<<<<<<<<<<<<<");
	dout(">>>>>>>>>>>>>>>>>>>>>>>   CWMIN: " + num2str(Global::aCWmin) + "        <<<<<<<<<<<<<<<<<<\n\n");
	// end of setting inputs

	/* read the look up tables */
	IO::read_packet_error_rate_lut();
	IO::read_symbol_error_rate_lut();


	/* setup the traffic generator and random number of generator */
#ifndef DETERMINISTIC
	for (auto& s : selected_stations) Global::seeds.push_back(std::random_device()());
#else
	for (auto& s : output[seed]) Global::seeds.push_back(str2long(s));
#endif

	for (int i = 0; i < output[payload_rate].size(); ++i)
	{
		auto s = output[payload_rate][i];
		if (i >= Global::station_count)
			Global::traffic_load.insert(std::pair<uint, float>(Global::station_count-1, str2double(s)));
		else
			Global::traffic_load.insert(std::pair<uint, float>(i, str2double(s)));
	}
	simultaneous_tx = Global::station_count * Global::chwindow;
	inters.reserve(Global::station_count - 1);

	/* build the map of station and IDs*/
	auto station_count = selected_stations.size();
	if (Global::traffic_load.size() < station_count || Global::txpowers.size() != station_count)
		error_out("traffic load: Input file wrong");

	for (uint sta = 0; sta < station_count; ++sta)
	{
		Global::sta_name_map[selected_stations[sta]] = sta;
	}

	/* Shortlist stations based on the selected stations in the input file */
	vector<pair<int, int>> indices, indices2;
	for (auto &selected_sta : selected_stations)
	{
		for (int row_idx = 0; row_idx < sta_names.size(); ++row_idx)
		{
			if (sta_names[row_idx][0] == selected_sta)
			{
				for (int col_idx = 1; col_idx < sta_names[row_idx].size(); ++col_idx)
				{
					for (auto &selected_sta2 : selected_stations)
					{
						if (sta_names[row_idx][col_idx] == selected_sta2)
						{
							indices.emplace_back(pair<int, int>(Global::sta_name_map[sta_names[row_idx][0]],
								Global::sta_name_map[sta_names[row_idx][col_idx]]));
							indices2.emplace_back(pair<int,int>(row_idx, col_idx));
						}
					}
				}
			}
		}
	}

	for (int i = 0; i < indices.size(); ++i)
	{
		auto norm_idx = indices[i];
		auto abs_idx = indices2[i];
		station_names[norm_idx.first][norm_idx.second] = sta_names[abs_idx.first][abs_idx.second];
		distance_table[norm_idx.first][norm_idx.second] = sta_distances[abs_idx.first][abs_idx.second];
		pathloss_table[norm_idx.first][norm_idx.second] = sta_pathlosses[abs_idx.first][abs_idx.second];
	}

	/* define connections*/
#ifndef AP_MODE
	for (uint i = 0; i < output[conns].size(); i += 2)
	{
		uint source = stoi(output[conns][i]);
		uint dest = stoi(output[conns][i + 1]);
		Global::connections.add(source, dest);
#else
	for (int sid = 0; sid < station_count; ++sid)
	{
		if (sid == Global::sta_name_map[Global::ap_station])
			continue;
		Global::connections.add2way(sid, Global::sta_name_map[Global::ap_station]);
#endif
	}

	mkdir(output_dir.c_str());
	path = output_dir + create_tstamp() + (Global::prop_factor > 1.0 ? "" : ("-L-" + num2str(Global::aCWmax)
		+ "-" + num2str(Global::aCWmin) )) + "/";
	mkdir(path.c_str());

	/* initialize the gui_map */
	gcellvector init(Global::produration);
	guimap.resize(station_count, init);

	/* create stations and their logs */
	for (uint sta = 0; sta < station_count; ++sta)
	{
		logs.stations.push_back(new Logger(sta, path, selected_stations[sta]));
		station_list.push_back(sptrStation(new Station(selected_stations[sta], station_names, distance_table, pathloss_table, &guimap[sta])));
	}
	/* remove the dead links from the downlink if uplink thinks AP is unreachable (inefficient) */
	umap<station_number, Antenna*> ant_list;
	for (auto s : station_list)
	{
		auto linkmap = s->getMacLayer()->getmap()->link;
		for (auto destination_info : linkmap)
		{
			auto destination = destination_info.first;
			if (destination_info.second.dead)
			{
				(*station_list[destination]->getMacLayer()->getmap())(s->getID()) = true;
			}
		}
		ant_list[s->getID()] = s->getPhyLayer()->getAntenna();
	}
	for (auto s : station_list) s->global_update(ant_list); // finally assign traffic for the stations

	/* clear the channel during the times of simulation */
	vector<sptrFrame> station(Global::produration, NULL);
	channel.resize(station_count, station);

	for (auto &source : station_list)
		proptable[source->getID()] = source->getAllPropagationDelays();

	vector<float> loads;
	for (auto sta : station_list)
	{
		auto a = mltimap2vector(Global::traffic_load, sta->getID());
		loads.insert(loads.end(), a.begin(), a.end());
	}

	auto filename =  "Summary [" + num2str(loads) + "]";
	logs.common = new Logger(path, filename);
	logs.common->print_input_file(input_file_contents, "seeds," + num2str(Global::seeds), num2str(Global::dot11ShortRetryLimit), seed);
}

void Program::phy_cca_indication(uint now, uint destination_id)
{
	uint size_ota = otalist.size();
	inters.clear();
	for (int i = sliding_window; i < size_ota; ++i)
	{
		auto pair = otalist[i];
		uint source_station = pair.source;
		if (source_station != destination_id)
		{
			uint a = pair.reltime + proptable.at(source_station).at(destination_id) + pair.dur;
			uint b = pair.reltime + proptable.at(source_station).at(destination_id);

			if (now < a	&& now >= b)
			{
				inters.push_back(source_station);
			}
		}
	}
}
void Program::update_end_time(int now)
{
	for (auto station : station_list)
	{
		if (station->active())
		{
			if ((long int)Global::produration - (long int)now < 1e6)
			{
				Global::produration += 1e6;
				end_time = Global::produration + 1;

				for (int sta = 0; sta < Global::station_count; ++sta)
				{
					channel[sta].resize(channel[sta].size() + 1e6, NULL);
#ifdef SHOWGUI
					guimap[sta].resize(guimap[sta].size() + 1e6);
#endif
				}
			}
			return;
		}
	}
	Global::produration = now;
}
void Program::print_progress(int now)
{
	if (now % 1000000 == 0)
	{
		log_throughput(now, total_data, station_list, per_second_thru);
		update_end_time(now);
	}

	int p = round(100 * now / (float)Global::simduration);
	if (p % 10 == 0)
	{
		if (progress_printed != p)
		{
			progress_printed = p;
			debugout("= ");
		}
	}
}
bool Program::transmit_frame(uint current_time, std::shared_ptr<Frame> &frame)
{
	uint mode;
	auto type = frame->subval();
	auto source = frame->getSource();
	auto destin = frame->getDest();
	auto duration = frame->getDuration();
	auto &station = station_list[source];

	if (current_time + duration > end_time) return false;

	switch (type)
	{
	/* think like this: if I send _PACKET: then I expect back mode = WHAT */
	case Global::_ACK: mode = rts_flag;
		break;
	case Global::_RTS: mode = cts_flag | ack_flag;
		break;
	case Global::_CTS: mode = dat_flag | rts_flag;	// could be RTS_X+1 without sending dataX if RTS is received
		break;										// or could be another staiton's RX which will cause isExpectedFrame = false;
	case Global::_DATA: mode = ack_flag;
	}

	for (uint i = 1; i < duration; ++i)
	{
		++current_time; //assign the same frame accross the timeline with the same characteristics.
		channel[frame->getSource()][current_time] = frame;
	}

	if (type == Global::_ACK)
	{
		station->resetPacketMode();
	}
	else station->setPacketMode(destin, mode);

	return true;
}
void Program::run()
{
	end_time = (ENDIT > 50000 ? Global::produration : ENDIT) + 1;

	/* simulator starts here */
	system_time = 0;
	for (uint &now = system_time; now < Global::produration; ++now)
	{
		if (now) print_progress(now);
		if (now == end_time)
			break;

		for (uint sta = 0; sta < station_list.size(); ++sta)
		{
			Station& station = *station_list[sta];
			phy_cca_indication(now, sta);
			auto energized = station.channel_update(inters);
			auto frame = channel[sta][now];
			station.update_timers(now, NULL);

			if (frame == NULL)
			{
				if (!energized)
				{
					station.reset_receiver(now, true);
				}
				frame = station.lookup_buffer(now, frame.get());
				channel[sta][now] = frame;
			}

			//-------- channel is clear ------------------------------
			if (!inters.size())
			{
				if (station.getAntennaMode() == RX)
				{
					//if there is no frame to transmit, then move on to the next station
					if (channel[sta][now] == NULL) continue;

					//here's where it gets into a TX state, and latches on to this stable state
					//even if receiving right now, so this will allow us to determine if a backoff needs to happen
					auto type = channel[sta][now]->subval();
					station.setTX(now, type == Global::_ACK || type == Global::_DATA);
					otalist.add(frame->getSource(), now, frame->getDuration()); //frame is now in the air
					if (!transmit_frame(now, frame)) return;
#ifdef SHOWGUI
					guimap[sta][now].add(new gui_frame_stat_tx{ frame->getSource(), frame->getDest(), frame->subval(),
						frame->getSequence(), frame->getFrag() });
#endif
				}
				else // if (station.getAntennaMode() == TX)
				{
					// ---- now transmit in a stable state ---
					if (channel[sta][now] == NULL)
					{
						station.setRX(now);
						auto type = channel[sta][now - 1]->subval();
						if (type != Global::_ACK)
						{
							station.update_timers(now, channel[sta][now - 1].get());
						}
						station.get_phy_indication().tx();
					}
					else
					{
						station.get_phy_indication().tx.start.b = false;
#ifdef SHOWGUI
						guimap[sta][now].add(new gui_frame_stat_tx{ frame->getSource(), frame->getDest(), frame->subval(),
							frame->getSequence(), frame->getFrag() });
#endif
					}
				}
			}
			else // channel is busy
			{
				if (energized)
				{
					station.set_DIFS_busy(now, Station::STAspace::BUSY);
					station.make_BCKOFF_busy(now);
				}

				if (station.getAntennaMode() == RX)
				{
					if (channel[sta][now] != NULL)
					{
						auto type = channel[sta][now]->subval();
						if (type != Global::_RTS || !energized)
						{
							station.reset_receiver();
							station.setTX(now, type == Global::_ACK || type == Global::_DATA);
							otalist.add(frame->getSource(), now, frame->getDuration()); //frame is now in the air
							if (!transmit_frame(now, frame)) return;
#ifdef SHOWGUI
							guimap[sta][now].add(new gui_frame_stat_tx{ frame->getSource(), frame->getDest(), frame->subval(),
								frame->getSequence(), frame->getFrag() });
#endif
						}
						else
						{
							channel[sta][now] = NULL; // frame still in the queue, just -1 retry number
							logs.stations[sta]->writeline(num2str(now) + " frame negated");
							station.frame_drop_check(now);
						}
					}
				}
				else // (station.getAntennaMode() == TX)
				{
					// immediately go into a receive mode after transmit done
					if (channel[sta][now] == NULL)
					{
						station.setRX(now);
						auto type = channel[sta][now - 1]->subval();
						if (type != Global::_ACK)
						{
							station.update_timers(now, channel[sta][now - 1].get());
						}
						station.get_phy_indication().tx();
					}
					// actively transmitting whether receiving in the same time or not.
					else
					{
						station.get_phy_indication().tx.start.b = false;
#ifdef SHOWGUI
						guimap[sta][now].add(new gui_frame_stat_tx{ frame->getSource(), frame->getDest(), frame->subval(),
							frame->getSequence(), frame->getFrag() });
#endif
					}
				}
				station.evaluate_channel(now, channel);
			}
		}
	}
}
void Program::summary()
{
	float endtime = end_time / (float)1000.0;
	logs.common->writeline("Overall simulation duration (ms)   :\t" + double2str(endtime, 2));
	logs.common->writeline("Last wireless channel utilization  :\tstation" + num2str(otalist.last().source) + ", " + num2str(otalist.last().reltime / 1000.0) + " ms");

	uint last_dequeue = 0;
	/* log station summary */
	for (auto &sta : station_list)
	{
		logs.common->writeline("\n\n");
		uint time = sta->prepare_summary();
		last_dequeue = max(last_dequeue, time);

		sta->summrize_sim(logs.common, endtime);
	}

	/* print throughput data */
	logs.common->writeline("\n================================= THOUGHPUT OVER TIME ===============================X");
	logs.common->writeline("time(s),throughput(Mbps)");

	logs.common->writeline("------------");
	if (per_second_thru.size())
	{
		for (auto dat : per_second_thru)
		{
			logs.common->writeline(num2str((int(dat.first / 1e6))) + "," + num2str(dat.second));
		}
		auto data = system_time > 1e6 ? total_data - prev(per_second_thru.end())->second : total_data;
		auto size = system_time > 1e6 ? (per_second_thru.size() - 1) : per_second_thru.size();
		logs.common->writeline("average-persec," + num2str(data / size));
		logs.common->writeline("average-fullsim," + num2str(total_data / (last_dequeue / 1e6)));
	}

	logs.common->writeline("total," + num2str(total_data));

	station_list.back()->unbuffer_ap_stats(logs.common);
}
gcelltable Program::getgui()
{
	dout("======================================== WORKING ON GUI NOW ==================================");
	return guimap;
}
