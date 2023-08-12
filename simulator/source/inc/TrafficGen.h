#pragma once
#include "Mac.h"
using namespace std;

typedef std::shared_ptr<RTS> sptrRTS;
typedef std::shared_ptr<CTS> sptrCTS;
typedef std::shared_ptr<DATA> sptrDATA;
typedef std::shared_ptr<ACK> sptrACK;
typedef std::shared_ptr<uint> uintptr;

/* store the event data along with destination id */
struct EventData
{
	uint sta;
	double time;

	struct less_than
	{
		inline bool operator()(const EventData& lhs, const EventData& rhs)
		{
			return lhs.time < rhs.time;
		}
	};

	struct equal_to
	{
		inline bool operator()(const EventData& lhs, const EventData& rhs)
		{
			return lhs.time == rhs.time;
		}
	};

	EventData(uint _sta, double _time) : sta(_sta), time(_time) {}
};


struct EventData_S
{
	vector<EventData> data;
	const size_t size() const
	{
		return data.size();
	}
	const uint& time(uint index) const
	{
		return data[index].time;
	}
	const uint& dest(uint index) const
	{
		return data[index].sta;
	}
};

class TrafficGenerator
{
private:
	unsigned long int seed;
	umap<station_number, float> medium_load, bytes_per_packet;
	uint slot_count;
	umap<station_number, double> busyf;
	std::mt19937_64& generator;
	std::vector<EventData>& eventdata;
public:
	TrafficGenerator(const uint phyid, const uivector& dests, const MCSDataManager& mcs_mapping, std::vector<EventData>& payloads, std::mt19937_64& gen);
	void generate_payload_events(const station_number& station, std::vector<EventData>& event_list_output);
	uint payload_size(station_number station);
	float getLoad(station_number station);
	uint getseed();
};

