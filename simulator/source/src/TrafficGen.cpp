#include "../inc/TrafficGen.h"



TrafficGenerator::TrafficGenerator(const uint phyid, const uivector& dests, const MCSDataManager& mcs_mapping, vector<EventData>& payloads, std::mt19937_64& gen) :
	seed(Global::seeds[phyid]),
	eventdata(payloads),
	generator(gen)
{
	generator = std::mt19937_64(seed);// generator_object;

	auto logger = logs.stations[phyid];
	float slots_per_s = 1e6 / (double)dot11a_slot_time;

	bool is_ap = dests.size() > 1 ? true : false; // AP specific

	auto traffic_loads = mltimap2vector(Global::traffic_load, phyid);
	uint count = 0;

	for (int i = 0; i < traffic_loads.size(); ++i)
	{
		medium_load[dests[i]] = mcs_mapping.at(dests[i]).dead == false ? traffic_loads[i] : 0;
	}

	slot_count = floor(Global::simduration / (double)dot11a_slot_time);

	for (auto& station : dests)
	{
		bytes_per_packet[station] = Global::data_pack_size;
		float packets_per_s = (1e6 * medium_load.at(station)) / (double)(8 * bytes_per_packet.at(station));
		busyf[station] = packets_per_s / slots_per_s;

		auto& utilization = busyf.at(station);

		if (0 < utilization && utilization <= 1.0)
		{
			logger->writeline("        TrafficGen: payloads are being queued to station " + num2str(station)
				+ ", busy: " + double2str(busyf.at(station) * 100, 2) + "%");
			generate_payload_events(station, eventdata);
		}
		else if (utilization > 1)
		{
			dout("TrafficGen: Invalid parameters. Busy ratio is more than 100%; sta " + num2str(phyid), true);
		}
		else// utilization == 0
		{
			logger->writeline("                    no traffic scheduled for this station [medium_load = "
				+ num2str(medium_load) + "]");
		}
	}

	if (is_ap)
	{
		std::sort(eventdata.begin(), eventdata.end(), EventData::less_than());
		eventdata.erase(std::unique(eventdata.begin(), eventdata.end(), EventData::equal_to()), eventdata.end());
	}

	logger->writeline("-------------------------------------------------------------------------------------");
	logger->writeline("                               EVENT SCHEDULER DATA                                  ");
	logger->writeline("-------------------------------------------------------------------------------------");
	logger->writeline("                                seed: " + num2str(seed));
	logger->writeline("                        format station-id:time-scheduled\n");

	for (auto& an_event : eventdata)
	{
		logger->write(num2str(an_event.sta) + ":" + num2str((uint)an_event.time) + ",");
	}
	logger->write("\n\n");

	logger->writeline("-------------------------------------------------------------------------------------");
	logger->writeline("                                     PROGRAM LOG                                     ");
	logger->writeline("-------------------------------------------------------------------------------------\n");
}
void TrafficGenerator::generate_payload_events(const station_number& sta, vector<EventData>& events_list_output)
{
	std::uniform_real_distribution<double> distribution(0.0, 1.0);

	for (uint slot = 0; slot < slot_count; ++slot)
	{
		double prob = distribution(generator);
		if (prob <= busyf.at(sta))
			events_list_output.emplace_back(sta, slot * dot11a_slot_time);
	}
}
uint TrafficGenerator::payload_size(station_number station)
{
	return bytes_per_packet.at(station);
}
float TrafficGenerator::getLoad(station_number station)
{
	return medium_load.at(station);
}
uint TrafficGenerator::getseed()
{
	return seed;
}
