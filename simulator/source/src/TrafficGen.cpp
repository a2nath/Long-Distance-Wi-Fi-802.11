#include "../inc/TrafficGen.h"

TrafficGenerator::TrafficGenerator(uint phyid, uivector &dests, McsManager *mcs_mapping) : seed(Global::seeds[phyid])
{
	std::mt19937_64 generator_object(seed);
	generator = generator_object;

	auto logger = logs.stations[phyid];
	float slots_per_s = 1e6 / (double)dot11a_slot_time;

	auto traffic_loads = mltimap2vector(Global::traffic_load, phyid);

	for (int i = 0; i < traffic_loads.size(); ++i)
	{
		medium_load[dests[i]] = !mcs_mapping->at(dests[i]).dead ? traffic_loads[i] : 0;
	}

	slot_count = floor(Global::simduration / (double)dot11a_slot_time);
	umap<station_number, uivector> payload_event_mapping;

	for (auto& station : dests)
	{
		bytes_per_packet[station] = Global::data_pack_size;
		float packets_per_s = (1e6 * medium_load.at(station)) / (double)(8 * bytes_per_packet.at(station));
		busyf[station] = packets_per_s / slots_per_s;
		if (busyf.at(station) > 1.0) dout("Busy fraction is more than 100%; sta " + num2str(phyid), true);

		payload_event_mapping[station] = generate_payload_events(station);
		logger->writeline("Payload queue to Station " + num2str(station) + " | busy: " + num2str(busyf.at(station) * 100) + "%");
		if (busyf.at(station) == 0) logger->writeline("No traffic scheduled for this station [medium_load = " + num2str(medium_load) + "]");
	}

	if (dests.size() > 1)
	{
		std::uniform_int_distribution<int> destinations(0, dests.size() - 1);
		int mx(0), count(0);

		for (auto& m : payload_event_mapping)
		{
			if (m.second.size() > mx) mx = m.second.size();
			count += m.second.size();
		}

		payload_events.reserve(count);
		uivector destination_vector, dest_index_vector;
		for (auto station_events : payload_event_mapping)
		{
			// merge all event vectors
			payload_events.insert(payload_events.end(), station_events.second.begin(), station_events.second.end());

			// build the destination vector
			uivector d(station_events.second.size(), station_events.first);
			destination_vector.insert(destination_vector.end(), d.begin(), d.end());
		}

		// sort both
		dest_index_vector.resize(payload_events.size());

		//populate the index vector with increasing vals
		for (unsigned i = 0; i < dest_index_vector.size(); ++i) dest_index_vector[i] = i++;

		std::sort(dest_index_vector.begin(), dest_index_vector.end(), // re-arrange index as per sorting permutation done on the events vector
			[&](const int& a, const int& b) { return (payload_events[a] < payload_events[b]); });
		std::sort(payload_events.begin(), payload_events.end());

		for (uint slot = 0; slot < payload_events.size(); ++slot)
			destination_address_distr.push_back(destination_vector[dest_index_vector[slot]]);

		// remove duplicates
		uivector::iterator it(payload_events.begin());
		uivector dup_index;
		for (int i = 0; i < payload_events.size(); ++i)
		{
			it = std::adjacent_find(it, payload_events.end());
			if (it != payload_events.end())
				dup_index.push_back(it - payload_events.begin());
			else
				break;
			++it;
		}
		for (int i = dup_index.size() - 1; i > -1; --i)
			destination_address_distr.erase(destination_address_distr.begin() + dup_index[i]);
		payload_events.erase(std::unique(payload_events.begin(), payload_events.end()), payload_events.end());
	}
	else
	{
		destination_address_distr.push_back(dests[0]);
		payload_events = payload_event_mapping.at(dests[0]);
	}
	dest_pointer = destination_address_distr.size() > 0 ? &destination_address_distr[0] : NULL;
	logger->writeline("Seed: " + num2str(seed));
	logger->writefor(payload_events, 10);
	logger->writefor(destination_address_distr, 10);

}
uivector TrafficGenerator::generate_payload_events(station_number station)
{
	uivector events;
	std::uniform_real_distribution<double> distribution(0.0, 1.0);
	if (busyf.at(station) == 0) return events;

	for (uint slot = 0; slot < slot_count; ++slot)
	{
		double prob = distribution(generator);
		if (prob <= busyf.at(station))
			events.push_back(slot * dot11a_slot_time);
	}

	return events;
}
uint TrafficGenerator::getDest()
{
	return destination_address_distr.size() > 1 ? *(dest_pointer++) : destination_address_distr[0];
}
uint TrafficGenerator::payload_size(station_number station)
{
	return bytes_per_packet.at(station);
}
float TrafficGenerator::getLoad(station_number station)
{
	return medium_load.at(station);
}
vector<uint> *TrafficGenerator::getActualTimes()
{
	return &payload_events;
}
std::mt19937_64 &TrafficGenerator::get_generator()
{
	return generator;
}
uint TrafficGenerator::getseed()
{
	return seed;
}
