#pragma once
#include "TrafficGen.h"
#include "gcommon.h"
#include <algorithm>

#define nav_interruptable
#define ack_flag 0b1000
#define dat_flag 0b0100
#define cts_flag 0b0010
#define rts_flag 0b0001

typedef bool flag;

class Station
{
private:
	string self_name;
	uint uniqueID;
	Location currentlocation;
	MacLayer * maclayer;
	PhyAdapter * phyadapter;
	Indication * phy_indication;
	std::vector<int> handshake;
	vector<station_number> dest_addresses;
	uint dot11ShortRetryLimit;
	bool ap_mode;
	bool sifs_running;
	flag apevents;
	flag cts_ack_wrong_pckt;
	flag cts_received;
	flag dat_int_reset;
	flag energized;
	std::random_device rd;
	std::mt19937_64 random_generator;

	/* profiling variables */
	Logger *logger;
	vector<unordered_map<string, double>> per_station_summary;
	std::map<uint, uint> combined_qlat, combined_qsize;
	float total_data, total_load, last_event, ave_lat, ave_size, max_lat, max_size;

	gcellvector *guiptr;
	umap<station_number, prop_del_us> rrt_map;

	/* station_number: <event-time, dequeue-time> */
	umap<station_number, std::map<uint, uint>> devent;

	/* station_number: <event-time, queue-size> */
	umap<station_number, std::map<uint, uint>> queue_size;
	umap<station_number, uint> dropped_count;
	typedef std::pair<retry_count, std::shared_ptr<Frame>> qpair;
	uint total_events_ququed, total_events_procc, total_events_dropped;
	unordered_map<uint, vector<uint>> events_queued;

	std::unique_ptr<Timer> cts_int_timer, dat_int_timer, ack_int_timer, rts_nav, cts_nav, difs_wait;
	std::unique_ptr<Backoff_Manager> backoff_timer;
	struct Buffer
	{
		qpair buf;
		void settime(rts_release_time time) { buf.second->change_time(nextslottime_us(time)); }
		/* Frame is delivered after receiving its corresponding CTS/ACK frame */
		void delivered() { buf.second = NULL; }
		/* Throws the frame from the buffer since it reached maximum retries.
		Same as delivered, but created separately for readability purposes */
		void discard() { delivered(); }
		void retry(int r) { buf.first += r; }
		std::shared_ptr<Frame> get(int time) { return (buf.second == NULL || buf.second->getTime() != time) ? NULL : buf.second; }
		void operator=(qpair* b) { buf.first = b->first; buf.second = b->second; }
		bool operator!=(std::shared_ptr<Frame>& frame) { return (buf.second == NULL || buf.second != frame) ? true : false; }
		bool operator==(std::shared_ptr<Frame>& frame) { return (buf.second == NULL || buf.second != frame) ? false : true; }
		bool operator()() { return buf.second != NULL && buf.first != 0; }

		int val_re() { return buf.second != NULL && buf.first != 0 ? buf.first : -1; }
		int abs_re() { return buf.second != NULL ? buf.first : -1; }
		int seqn() { return buf.second != NULL ? buf.second->getSequence() : -1; }
		int getdest() { return buf.second != NULL ? buf.second->getDest() : -1; }
		~Buffer() {}
	} txBuffer;

	struct queue_structure
	{
		std::map<rts_queued_time, qpair> buffer;
		qpair* getpair()
		{
			return &buffer.begin()->second;
		}
		void add(uint retry_count, shared_ptr<Frame> &frame)
		{
			buffer[frame->getTime()] = qpair{ retry_count, frame };
		}
		void delivered()
		{
			buffer.erase(buffer.begin()); // this is always true - beginning has to be earliest frame
		}
		void discard() { delivered(); }
		size_t size() { return buffer.size(); }
		uint retries() { return getpair()->first; }
		void retry(int recount) { getpair()->first += recount; }
		qpair* get(uint i)
		{
			if (buffer.empty() || getpair()->second->getTime() > i)
				return new qpair(0, NULL);
			else
				return getpair();
		}
		~queue_structure() {}
	} txQueue;
	struct expected_frame { int sta; uint mode; } next_expected_frame, prev_expected_frame;

	struct handle_non_seq_cts
	{
		bool recover;
		int sequence;
		void reset() { recover = false; sequence = -1; }
	} dropped_latch;

	/* update backoff */
	flag wait_for_timers;
	float SER;
	flag difs_finished;
	uint datasize;
	uint transmitted_frame_count;
	TrafficGenerator * trafficgen;
	EventData_S actual_times;

	uint rts_idx;
	uint simtime;
	uint mpdu_header_size;

	void decode_frame(uint current_time, Frame * rx_frame, vector<sptrFrame>& wireless_channel);
	const uint get_next_dest_id() const;
	uint get_next_rts_time();
	bool trafficPresent();
	bool exp_cts_ack_resp(int &type);
	bool isExpectedFrame(Frame * frame);
	void queue_update(std::shared_ptr<Frame>& response);
	bool isPacketMode(int station, uint seq, uint mode);
	int txqueue_seq();
	bool cts_accept(uint sequence);
	void tx_response(uint current_time, Frame * received_frame, vector<sptrFrame>& cellrow);
	/* station state */
	float system_noise();
	uint getProp(uint station_destination);
	Location getLocation();
	uint get_data_count(station_number station);

	/* timer functions, timers.cpp */
	void update_gui_timers(uint current_time);
	void rts_nav_start(uint current_time, Frame* frame);
	void rts_nav_update(uint current_time);
	void cts_nav_start(uint current_time, Frame* frame);
	bool is_nav_rts_expired(uint current_time);
	bool is_nav_cts_expired(uint current_time);
	bool phy_cs_active(uint current_time);
	bool virtual_cs_active(uint current_time);

	bool is_DIFS_expired(uint current_time);
	bool is_DIFS_busy();


	void cts_int_start(uint current_time);
	void dat_int_start(uint current_time);
	void ack_int_start(uint current_time);
	bool is_cts_expired(uint current_time);
	bool is_dat_expired(uint current_time);
	bool is_ack_expired(uint current_time);

	void difs_set(flag flag);
	void difs_reset(uint current_time, flag setstate = true);
	void difs_and_backoff(uint current_time);
	bool is_backoff_expired(uint current_time);
	void backoff_resume(uint current_time);
	void backoff_start(uint current_time);
	void backoff_reset(uint current_time);

	/* traffic functions */
	void queue2buffer(uint current_time);
	void retry_offset(retry_count count);
	bool station_idle();
	std::shared_ptr<Frame> tx_dequeue(uint current_time);
	sptrRTS genRTS(const uint current_time);
	void set_wait(flag flag);
	bool response_waiting(uint &current_time);
public:
	Station(const string name, const map<uint, map<uint, string>>& station_names, const map<uint, map<uint,
		double>>& distance, const map<uint, map<uint, double>>& pathloss, gcellvector *guitable);
	~Station();

	uint getID();
	float get_data_bytes(uint station);
	std::shared_ptr<Frame> lookup_buffer(uint current_time, Frame * frame);
	void set_link_to_dead(station_number station);
	MacLayer * getMacLayer();
	PhyAdapter * getPhyLayer();
	Indication& get_phy_indication();
	bool channel_update(vector<station_number>& interferers);
	bool sifs();
	transciever_mode getAntennaMode();
	umap<uint, uint> getAllPropagationDelays();

	struct STAspace
	{
		static const bool DONE = true;
		static const bool IDLE = false;
		static const bool BUSY = true;
		static const bool UNFIN = false;
	};

	/* timer updates */
	void global_update(umap<station_number, Antenna*>& ant_list);
	void update_timers(uint current_time, Frame *frame);
	void set_DIFS_busy(uint current_time, flag state);
	void make_BCKOFF_busy(uint current_time);
	void difs_start(uint current_time);
	void difs_unset();
	void sifs_unset();

	/* recevier updates */
	void evaluate_channel(uint current_time, vector<vector<sptrFrame>>& wireless_channel);
	void reset_receiver(uint current_time = 0, bool special = false);
	void setRX(uint present_time);
	bool frame_drop_check(uint &current_time);

	/* transmitter updates */
	void setPacketMode(int station, uint mode = rts_flag | dat_flag);
	expected_frame& getPacketMode();
	void setTX(uint present_time, bool isACK);
	void resetPacketMode();

	/* profiler functions */

	/* gathers data from the network and station and returns the last event */
	uint prepare_summary();
	void summrize_sim(Logger* common, float endtime);
	void overall_summary(Logger * logger, float endtime);
	float total_data_transferred();
	void buffer_ap_stats(string input);
	void unbuffer_ap_stats(Logger* logger);
	bool active();
};
