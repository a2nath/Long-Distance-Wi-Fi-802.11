#pragma once
#include "Mac.h"
using namespace std;

typedef std::shared_ptr<RTS> sptrRTS;
typedef std::shared_ptr<CTS> sptrCTS;
typedef std::shared_ptr<DATA> sptrDATA;
typedef std::shared_ptr<ACK> sptrACK;
typedef std::shared_ptr<uint> uintptr;


class TrafficGenerator
{
private:
	unsigned long int seed;
	umap<station_number, float> medium_load, bytes_per_packet;
	uivector payload_events;
	uivector destination_address_distr;
	uint *dest_pointer;
	uint slot_count;
	umap<station_number, double> busyf;
	std::mt19937_64 generator;
public:
	TrafficGenerator() {}
	TrafficGenerator(uint phyid, uivector &destinations, McsManager *mcs_mapping);
	uivector generate_payload_events(station_number station);
	uint payload_size(station_number station);
	float getLoad(station_number station);
	uint getDest();
	std::vector<uint> *getActualTimes();
	std::mt19937_64 & get_generator();
	uint getseed();
};

