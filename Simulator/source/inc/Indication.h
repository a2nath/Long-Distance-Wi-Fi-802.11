#pragma once
#include "Antenna.h"
#include "Frames.h"
#include "PhyRate.h"

class Indication
{
	station_number self_id;
	doublevector signal;
	doublevector noisal;
	int max_signal_source;
	uint rxed_frame_dur;
	Frame * fully_decoded;
	float ei_system_noise; //dBm
	bool preamble_found;
	double detection_threshold_W;
	umap<station_number, prop_del_us> propmap;
	struct cell { uint source, destination, type, sequence, frag; };
	struct outs { Frame* frame; std::vector<cell> guiinfo; bool energized; outs() : energized(false), frame(NULL) {} };
	struct booltimer
	{
		bool b; uint time;
		bool operator()(bool b, uint time) { return this->b == b && this->time == time; }
		void operator=(const booltimer &bt) { this->b = bt.b, this->time = bt.time; }
	};
	umap<station_number, float> rxpowers;
	transciever_mode *ant_state;

public:
	struct transmit_state
	{
		booltimer start, end;
		void operator()() { start = { false, UINT_MAX }, end = { false, UINT_MAX }; }
	} tx, rx;
	Indication(uint station_ID, transciever_mode *state, WirelessChannel *channel)
		: preamble_found(false), self_id(station_ID), ant_state(state), max_signal_source(-1),
		rxed_frame_dur(0), fully_decoded(NULL), propmap(channel->getPropgationMap()),
		ei_system_noise(channel->get_effective_noise()),
		detection_threshold_W(dBm2W(channel->get_effective_noise_dBm() + Global::PER_thrsh.at("BPSK12")))
	{
		tx();
		rx();
	}
	void global_update(umap<station_number, Antenna*> &ant_list)
	{
		for (auto ant : ant_list)
		{
			if (ant.first == self_id) continue;
			rxpowers[ant.first] = dBm2W(ant.second->getPower() + ant.second->getChannel()->get_H_factor(self_id));
		}
	}
	double signal_ave()
	{
		return ave(signal);
	}
	double noisal_ave()
	{
		return ave(noisal);
	}
	void cca_reset() // PHY-CCARESET.request
	{
		fully_decoded = NULL;
		max_signal_source = -1;
		rxed_frame_dur = 0;
		preamble_found = false;
		signal.clear();
		noisal.clear();
		rx();
	}

	bool isOverThld(vector<station_number> &interferers)
	{
		for (auto station : interferers)
		{
			if (rxpowers.at(station) > detection_threshold_W)
				return true;
		}
		return false;
	}

	outs rx_state_machine(uint current_time, vector<vector<sptrFrame>>& wireless_channel)
	{
		outs rx_outputs;
		auto &gui = rx_outputs.guiinfo;
		int mstation_by_sigstrength_ = -1;
		umap<station_number, float> signal_strength;

		/*store how far back in time to look in the channel datastructure: uint station, uint signal_strength */
		for (auto power : rxpowers)
		{
			int back_in_time = current_time - propmap.at(power.first);
			Frame * frame = back_in_time > -1 ? wireless_channel[power.first][back_in_time].get() : NULL;

			if (frame != NULL)
			{
				signal_strength[power.first] = power.second;
#ifdef SHOWGUI
				gui.push_back({ frame->getSource(), frame->getDest(), frame->subval(), frame->getSequence(), frame->getFrag() });
#endif
			}
		}

		if (*ant_state == TX)
			return rx_outputs;

		/* find max in the signal strength map */
		vector<float> noise;
		auto max = detection_threshold_W;
		if (signal_strength.find(max_signal_source) == signal_strength.end() && preamble_found)
			preamble_found = false;

		if (!preamble_found)
		{
			for (auto& signal : signal_strength)
			{
				if (max < signal.second)
				{
					max = signal.second;
					mstation_by_sigstrength_ = signal.first;
				}
			}

			if (signal_strength.size() > 1 && mstation_by_sigstrength_ > -1)
			{
				for (auto& signal : signal_strength)
				{
					if (signal.first != mstation_by_sigstrength_)
					{
						noise.push_back(signal.second);
					}
				}
			}
		}

		/* populate signal, max station can change depending on who shows up */
		if (!preamble_found && mstation_by_sigstrength_ < 0)
		{
			/* belore detection threshold */
			return rx_outputs;
		}
		else if (!preamble_found && mstation_by_sigstrength_ != max_signal_source)
		{
			max_signal_source = mstation_by_sigstrength_; //this object changes because max source changes.
														  //if (!signal_strength.size()) return;
			float SINR;
			int back_in_time = current_time - propmap.at(max_signal_source);
			Frame * frame = wireless_channel[max_signal_source][back_in_time].get();
			auto previous_frame = wireless_channel[max_signal_source][back_in_time - 1].get();
			bool isPreamble = previous_frame == NULL || previous_frame->subval() != frame->subval();

			auto real_idx = frame->get_mcs_idx();
			if (signal_strength.size() > 1)
			{
				SINR = ((int)(lin2dB(max / ((noise.size() > 1 ? sum(noise) : noise[0]) + ei_system_noise)) * (double)100.0)) / (double)100.0;
			}
			else
			{
				SINR = ((int)(lin2dB(max / ei_system_noise) * (double)100.0)) / (double)100.0;

			}
			auto noisy_idx = get_sinr2idx(SINR);

			preamble_found = noisy_idx >= real_idx && isPreamble;
			rx_outputs.energized = true;
			fully_decoded = frame;
			rxed_frame_dur = frame->getDuration();
			signal.clear();
			noisal.clear();
			rx.start = { true,current_time };
			signal.resize(rxed_frame_dur);
			noisal.resize(rxed_frame_dur);
			signal[--rxed_frame_dur] = signal_strength.at(max_signal_source);
		}
		else if (signal_strength.size())/* station doesn't change, no need to flush the signal/noisal vectors */
		{
			rx_outputs.energized = true;
			rx.start.b = false;
			signal[--rxed_frame_dur] = signal_strength.at(max_signal_source);
		}

		/* populate noise */
		for (auto &signal : signal_strength)
		{
			if (signal.first == max_signal_source)
				continue;
			noisal[rxed_frame_dur] += signal.second;
		}

		rx_outputs.frame = !rxed_frame_dur ? fully_decoded : NULL;
		return rx_outputs;
	}
};
