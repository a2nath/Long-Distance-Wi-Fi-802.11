#pragma once
#include "common.h"
#include "Frames.h"
#include <chrono>
#include <math.h>
#define OFF -1

class Backoff
{
private:
	uint sta;
	uint *p_window;
	vector<uint> max_windows;
	std::mt19937_64 generator;
	std::vector<std::vector<uint>> times;
public:
	Backoff(uint station_id, std::mt19937_64 &gen_object);
	void success();
	void fail();
	uint gen_time(uint time);
};

struct Backoff_Manager
{
	struct slotsystem { uint slot; bool suspend; } slotmarked;
	uint id;
	int slots;
	int start_time, end_time;
	std::random_device rd;
	Backoff *botimer;
	std::mt19937_64 generator;

	Backoff_Manager(uint station_id, std::mt19937_64 &generator_object);
	~Backoff_Manager();
	void retry_exceeded();
	void success();
	bool running(uint current_time);
	void reset();
	void update(uint time);
	void make_boff_busy(uint time);
	int time();
	bool expired(int current_time);
	void resume(uint time);
	void start(uint time);
	uint getslots(uint time);
	bool inactive(int current_time);
};

struct Timer {
	int start_time = OFF;
	int end_time = OFF;

	Timer() {}
	virtual void reset()
	{
		end_time = OFF;
	}
	virtual bool running(int current_time = -1)
	{
		return end_time != OFF;
	}
	int time()
	{
		return end_time;
	}
	virtual bool expired(int current_time)
	{
		return current_time >= end_time;
	}
	virtual bool inactive(int current_time)
	{
		return (start_time == end_time == OFF) || (expired(current_time) && !running());
	}

	virtual void start(uint time) {}
	virtual void start(uint time, Frame* frame) {}
};
struct tDAT : Timer
{
	virtual void start(uint time) override
	{
		if (!expired(time)) error_out("DAT timer is not expired");
		end_time = nextslottime_us(time + DATTimeoutInterval);
	}
};
struct tACK : Timer
{
	virtual void start(uint time) override
	{
		if (!expired(time)) error_out("ACK timer is not expired");
		end_time = nextslottime_us(time + ACKTimeoutInterval);
	}
};
struct tCTS : Timer
{
	virtual void start(uint time) override
	{
		if (!expired(time)) error_out("CTS timer is not expired");
		end_time = nextslottime_us(time + CTSTimeoutInterval);
	}
};
/* RTS NAV TIMER */
struct tRTSNAV : Timer
{
	uint end_time_tout = OFF; //timeout value in case the station doesn't see CTS frame that comes after previously detected RTS
	virtual void start(uint time, Frame* rts_frame) override
	{
		if (!expired(time))
		{
			dout("RTS-NAV update at " + num2str(time) + ", S:" + num2str(rts_frame->getSource()) + " D:" + num2str(rts_frame->getDest()));
		}

		start_time = time + 1;
		auto rts_dur = rts_frame->getDuration();
		auto data_ppdu_size = rts_frame->get_dat_duration();
		auto cts_dur = rts_dur;
		auto ack_dur = rts_dur;
		auto cts_start = nextslottime_us(start_time + 1 + dot11a_sifs);
		auto data_start = nextslottime_us(cts_start + cts_dur + 1 + dot11a_sifs);
		auto ack_start = nextslottime_us(data_start + data_ppdu_size + 1 + dot11a_sifs);
		end_time_tout = start_time + rts_nav_timeout_interval + cts_dur;
		end_time = ack_start + ack_dur + 1;
	}
	bool running(int current_time)
	{
		return start_time <= current_time && start_time != OFF ? true : false;
	}
	bool expired(int current_time) override
	{
		return current_time >= end_time || current_time >= end_time_tout;
	}
	bool timeout(uint time)
	{
		return time > end_time_tout;
	}
	void tout_update(uint time)
	{
		end_time_tout = INFINITE;
	}
	// TODO shouldn't be infinite, it should be end time of the CTS so that the RTS nav can get update even after that in long dist model
	void nav_update(uint cts_end)
	{
		end_time = cts_end;
	}
	void reset()
	{
		start_time = end_time = end_time_tout = OFF;
	}
	virtual bool inactive(int current_time) override
	{
		return (start_time == end_time == OFF) || (expired(current_time) && !running(current_time));
	}
};
/* CTS NAV TIMER */
struct tCTSNAV : Timer
{
	virtual void start(uint time, Frame* cts_frame) override
	{
		if (!expired(time))
		{
			dout("CTS-NAV update at " + num2str(time) + ", S:" + num2str(cts_frame->getSource()) + " D:" + num2str(cts_frame->getDest()));
		}

		start_time = time + 1;
		auto cts_dur = cts_frame->getDuration();
		auto data_ppdu_size = cts_frame->get_dat_duration();
		auto ack_dur = cts_dur;
		auto data_start = nextslottime_us(start_time + 1 + dot11a_sifs);
		auto ack_start = nextslottime_us(data_start + data_ppdu_size + 1 + dot11a_sifs);
		end_time = ack_start + ack_dur + 1;
	}
	bool running(int current_time)
	{
		return start_time <= current_time && start_time != OFF ? true : false;
	}
	void nav_update(uint rts_end)
	{
		if (rts_end > end_time)
		{
			end_time = rts_end;
		}
	}
	void reset()
	{
		start_time = end_time = OFF;
	}
	virtual bool inactive(int current_time) override
	{
		return (start_time == end_time == OFF) || (expired(current_time) && !running(current_time));
	}
};
/* DIFS TIMER */
struct dot11_difs : Timer
{
	bool medium_busy;
	void reset()
	{
		start_time = OFF;
		end_time = OFF;
	}
	bool running(int current_time) override
	{
		return start_time <= current_time && start_time != OFF ? true : false;
	}
	virtual void start(uint time) override
	{
		if (!expired(time)) error_out("DIFS timer is not expired");
		medium_busy = false;
		start_time = time;
		end_time = start_time + dot11a_difs * 1;
	}
	virtual bool inactive(int current_time) override
	{
		return (start_time == end_time == OFF) || (expired(current_time) && !running(current_time));
	}
};