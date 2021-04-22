#pragma once
#include "common.h"
#include <set>

enum s{ none = 0, difs, cts_timeout, dat_timeout, ack_timeout, backoff, nav_rts, nav_cts, nav_data };
struct wait_state
{
	std::set<s> buf;
	void operator=(s state) {if (buf.find(state) == buf.end()) buf.insert(state); }
	bool operator==(s state) { for (auto &s : buf) if (s == state) return true; return false; }
	bool operator>(s state) { for (auto &s : buf) if (s > state) return true; return false; }
};
struct gtimer
{
	std::vector<uint> timer_end;
	void operator=(int time) { timer_end.push_back(time); }
	uint operator[](uint idx) { return timer_end.size() > idx ? timer_end[idx] : timer_end[0]; }
};
struct gui_frame_stat {
	uint source;
	uint destination;
	uint subvalue;
	transciever_mode mode;
	uint sequence_count;
	uint fragment;
	wait_state wstates;
	gtimer timer_end;
	bool discarded;
	gui_frame_stat(uint s, uint d, uint subval, transciever_mode m, uint seqnum, uint frag)
		: source(s), destination(d), subvalue(subval), mode(m), sequence_count(seqnum),
		fragment(frag), discarded(true) {}
	uint gettimer(uint i)
	{
		return timer_end[i];
	}
};
struct gui_frame_stat_rx : gui_frame_stat
{
	gui_frame_stat_rx(uint s, uint d, uint subval, uint seqnum, uint frag) :
		gui_frame_stat(s, d, subval, RX, seqnum, frag) {}
};
struct gui_frame_stat_tx : gui_frame_stat
{
	gui_frame_stat_tx(uint s, uint d, uint subval, uint seqnum, uint frag, transciever_mode m = TX) :
		gui_frame_stat(s, d, subval, m, seqnum, frag) {}
};
struct gcell
{
	gui_frame_stat *transmitter = NULL, *receiver = NULL;
	gcell() {}
	void add(gui_frame_stat* obj)
	{
		obj->mode == TX ? transmitter = obj : receiver = obj;
	}
	wait_state* getmode()
	{
		if (transmitter == NULL)
			transmitter = new gui_frame_stat_tx{ 0, 0, 0, 0, 0, IDLE};
		return &transmitter->wstates;
	}
	gtimer* gettimer()
	{
		if (transmitter == NULL)
			transmitter = new gui_frame_stat_tx{ 0, 0, 0, 0, 0, IDLE };
		return &transmitter->timer_end;
	}
};
typedef std::vector<gcell> gcellvector;
typedef std::vector<gcellvector> gcelltable;