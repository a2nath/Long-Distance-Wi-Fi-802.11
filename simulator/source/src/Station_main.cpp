#include <iostream>
#include "../inc/Station.h"

uint Station::id = 0;
vector<string> ap_prints;

vector<unordered_map<string, double>> per_station_summary;
std::map<uint, uint> combined_qlat, combined_qsize;
float ave_lat = 0, ave_size = 0, max_lat = 0, max_size = 0;


Station::Station(const string name, const map<uint, map<uint, string>>& station_names,
	const map<uint, map<uint, double>>& distance, const map<uint, map<uint, double>>& pathloss, gcellvector *guitable)
	: uniqueID(Global::sta_name_map[name]), self_name(name), difs_finished(false), wait_for_timers(false), guiptr(guitable),
	trafficgen(NULL), actual_times(NULL), rts_idx(UINT_MAX), ap_mode(self_name == Global::ap_station), sifs_running(false),
	cts_ack_wrong_pckt(false), dat_int_reset(false), cts_received(false)
{
	logger = logs.stations[this->uniqueID];
	dot11ShortRetryLimit = Global::dot11ShortRetryLimit;
	resetPacketMode();
	datasize = Global::data_pack_size;
	apevents = Global::adapt_int_tout && ap_mode ? true : false;
	dest_addresses = Global::connections.dest(uniqueID);
	handshake.resize(Global::station_count, -1);
	for (auto d : dest_addresses) dropped_count[d] = 0;

	maclayer = new MacLayer(station_names, distance, pathloss, name);
	phyadapter = maclayer->getPhyLayer();
	phy_indication = phyadapter->get_phy_ind();
	if (apevents)
	{
		for (auto sta : dest_addresses)
			rrt_map[sta] = maclayer->getPhyLayer()->getAntenna()->getChannel()->getPropDelay(sta) * 2
			+ (dot11a_slot_time * 3) - CTSTimeoutInterval;
	}

	RTS frame = RTS(0, 0, 0, 0, 0, 0);
	mpdu_header_size = frame.get_header_size();
	cts_int_timer = unique_ptr<tCTS>(new tCTS());
	dat_int_timer = unique_ptr<tDAT>(new tDAT());
	ack_int_timer = unique_ptr<tACK>(new tACK());
	difs_wait = unique_ptr<dot11_difs>(new dot11_difs());
	rts_nav = unique_ptr<tRTSNAV>(new tRTSNAV());
	cts_nav = unique_ptr<tCTSNAV>(new tCTSNAV());
}
void Station::queue_update(std::shared_ptr<Frame> &response)
{
	if (response->getDest() == response->getSource())
		error_out("source same as destination");
	int retry_first_attempt_failed = 1;


	if (phy_indication->rx.start.time <= response->getTime()
		|| phy_indication->tx.start.time <= response->getTime()
		|| txQueue.size() || !station_idle() || wait_for_timers)
		--retry_first_attempt_failed;

	txQueue.add(dot11ShortRetryLimit + retry_first_attempt_failed, response);
}
void Station::retry_offset(retry_count count)
{
	if (!txBuffer()) error_out("queue empty");
	txBuffer.retry(count);
}
void Station::reset_receiver(uint current_time, bool special)
{
	if (!cts_received && special && response_waiting(current_time) && phyadapter->antenna_state == RX && !phy_cs_active(current_time))
	{
		frame_drop_check(current_time);
		resetPacketMode();
	}
	phy_indication->cca_reset();
}
void Station::global_update(umap<station_number, Antenna*> &ant_list)
{
	/* if traffic exists for this stations, then define the traffic parameters, otherwise they stay NULL */
	if (!dest_addresses.empty())
	{
		trafficgen = new TrafficGenerator(uniqueID, dest_addresses, maclayer->getmap());
		actual_times = trafficgen->getActualTimes();
		random_generator = trafficgen->get_generator();
		if (actual_times->size()) rts_idx = 0;
	}
	phy_indication->global_update(ant_list);
	backoff_timer = unique_ptr<Backoff_Manager>(new Backoff_Manager(uniqueID, random_generator));
}
uint Station::get_data_count(station_number station)
{
	return devent.find(station) != devent.end() ? devent.at(station).size() : 0;
}
float Station::get_data_bytes(uint station)
{
	return get_data_count(station) * (trafficgen != NULL ? trafficgen->payload_size(station) : 0);
}
Station::~Station()
{
	delete trafficgen;
	delete phyadapter;
	delete maclayer;
}
uint Station::getID()
{
	return uniqueID;
}
bool Station::channel_update(vector<station_number> &interferers)
{
	energized = interferers.size() ? phyadapter->get_phy_ind()->isOverThld(interferers) : false;
	phyadapter->getAntenna()->set_energized(energized);
	return energized;
}
PhyAdapter * Station::getPhyLayer()
{
	return phyadapter;
}
MacLayer * Station::getMacLayer()
{
	return maclayer;
}
umap<uint, uint> Station::getAllPropagationDelays()
{
	return phyadapter->getAntenna()->getChannel()->getPropgationMap();
}
void Station::setTX(uint now_time, bool timer_interrupt)
{
	if (timer_interrupt)
	{
		if (!backoff_timer->inactive(now_time))
		{
			backoff_reset(now_time);
			dout(num2str(now_time) + " Trying to cancel BO", true);
		}
		else if (!difs_wait->inactive(now_time))
		{
			difs_wait->reset();
			dout(num2str(now_time) + " Trying to cancel DIFS", true);
		}
		else if (virtual_cs_active(now_time))
		{
			rts_nav->reset();
			cts_nav->reset();
		}
	}
	else if (!is_backoff_expired(now_time)) error_out("Now we have a problem with transmitting when backoff");

	phy_indication->tx.start = { true, now_time };
	phyadapter->antenna_state = TX;
}
void Station::setRX(uint now_time)
{
	cts_received = false;
	phy_indication->tx.end = { true, now_time };
	phyadapter->antenna_state = RX;
}
int Station::txqueue_seq()
{
	return txQueue.size() ? txQueue.getpair()->second->getSequence() : -1;
}
bool Station::cts_accept(uint sequence)
{
	bool response = false;
	if (dropped_latch.recover == true && dropped_latch.sequence == sequence)
		response = true;
	dropped_latch.reset();
	return response;
}
bool Station::isPacketMode(int station, uint sequence, uint mode)
{
	/* not in the middle of an exchange */
	auto packet_mode_is_reset = next_expected_frame.sta == -1;
	if (packet_mode_is_reset)
	{
		if ((prev_expected_frame.mode & mode) == cts_flag)
		{
			if ((int)sequence == txqueue_seq() || cts_accept(sequence))
			{
				next_expected_frame = prev_expected_frame;
				prev_expected_frame = { -1, rts_flag | dat_flag };
				return true;
			}
			else return false;
		}
		else if ((prev_expected_frame.mode & mode) == ack_flag)
		{
			prev_expected_frame = { -1, rts_flag | dat_flag };
			return true;
		}
		else if ((prev_expected_frame.mode & mode) == dat_flag)
		{
			next_expected_frame = prev_expected_frame;
			prev_expected_frame = { -1, rts_flag | dat_flag };
			auto expected = (int)sequence == handshake.at(station);
			if (!expected) error_out("data sequence number not correct");
			return expected;
		}
		else
		{
			auto expected = (next_expected_frame.mode & mode) == rts_flag && (int)sequence >= handshake.at(station);
			next_expected_frame.sta = expected ? station : -1;
			return expected;
		}
	}
	/* in the middle of an exachange */
	else
	{
		auto a = next_expected_frame.sta == station;
		auto b = (next_expected_frame.mode & mode) == rts_flag && (int)sequence == handshake.at(station);
		auto c = (next_expected_frame.mode & mode) == dat_flag && (int)sequence == handshake.at(station);
		auto d = (next_expected_frame.mode & mode) == cts_flag && (int)sequence == txqueue_seq();
		auto e = (next_expected_frame.mode & mode) == ack_flag;
		return a && (b || c || d || e);
	}
}
bool Station::response_waiting(uint &current_time)
{
	return (getPacketMode().mode & cts_flag) == cts_flag || (getPacketMode().mode & ack_flag) == ack_flag;
}
/* I don't know about this */
bool Station::exp_cts_ack_resp(int &frame_type)
{
	auto mode = getPacketMode().mode;
	return (frame_type == Global::_CTS && (mode & cts_flag) == cts_flag) ||
		(frame_type == Global::_ACK && (mode & ack_flag) == ack_flag);
}
void Station::resetPacketMode()
{
	if (!(next_expected_frame.sta < 0))
		prev_expected_frame = next_expected_frame;
	setPacketMode(-1);
}
void Station::setPacketMode(int station, uint mode)
{
	next_expected_frame = { (int)station, mode };
}
Station::expected_frame& Station::getPacketMode()
{
	return next_expected_frame;
}
/* When next expected frame struct is sta = -1 */
bool Station::station_idle()
{
	return next_expected_frame.sta < 0;
}
/*
Note:
Take note that the connections between stations is 1:1 and the assumption is that
the frames are only going to appear from one station and therefore, one source.
Limitations of this simulator
*/
bool Station::isExpectedFrame(Frame * frame)
{
	auto type = frame->subval();
	auto source = frame->getSource();
	auto sequence = frame->getSequence();
	bool response = false;

	if (frame->getDest() == this->uniqueID)
	{
		if (type == Global::_RTS && isPacketMode(source, sequence, rts_flag))
		{
			response = true;
		}
		else if (type == Global::_CTS && isPacketMode(source, sequence, cts_flag))
		{
			response = true;
		}
		else if (type == Global::_DATA && isPacketMode(source, sequence, dat_flag))
		{
			response = true;
		}
		else if (type == Global::_ACK && isPacketMode(source, sequence, ack_flag))
		{
			response = true;
		}
	}
	return response;
}
sptrRTS Station::genRTS(const uint current_time)
{
	sptrRTS response_frame = NULL;
	uint source, destination, seqNum;
	if (get_next_rts_time() > current_time) return NULL;

	source = this->uniqueID;
	destination = trafficgen->getDest();
	seqNum = rts_idx;
	auto mcs = 0;
	auto data_dur = data_size(Global::data_pack_size + mpdu_header_size, maclayer->mcs_map(destination));

	response_frame = sptrRTS(new RTS(current_time, source, seqNum, destination, mcs, data_dur));
	auto casted_frame = shared_ptr<Frame>(response_frame);
	queue_update(casted_frame);
	++rts_idx;
	logger->writeline(num2str(current_time) + " qsize: " + num2str(txQueue.size()) + ", RTS add, seq" + num2str(seqNum));

	queue_size[destination][current_time] = txQueue.size();
	events_queued[destination].emplace_back(current_time);

	return response_frame;
}
void Station::queue2buffer(uint current_time)
{
	genRTS(current_time);

	if (txBuffer.buf.second == NULL && txQueue.size())
	{
		auto queue_response = txQueue.get(current_time);
		if (queue_response->second != NULL)
		{
			txBuffer = queue_response;
		}
	}
}
/* Reminder that the txbuffer can contain both RTS and DATA */
std::shared_ptr<Frame> Station::tx_dequeue(uint current_time)
{
	if (nextslottime_us(current_time) != current_time) return NULL;

	auto frame = txBuffer.get(current_time);
	if (frame)
	{
		retry_offset(-1);
		logger->writeline(num2str(current_time) + " qsize: " + num2str(txQueue.size())
			+ " queue-time " + num2str(actual_times->at(txBuffer.seqn())) + " at RTS release(" + num2str(frame->getDest()) + "), seq"
			+ num2str(txBuffer.seqn()) + (txBuffer.abs_re() == dot11ShortRetryLimit ? ", try " : ", retry ") + num2str(txBuffer.abs_re()));
		return frame;
	}
	else return NULL;
}
std::shared_ptr<Frame> Station::lookup_buffer(uint current_time, Frame* frame)
{
	if (wait_for_timers || !txBuffer() || (txBuffer() && !station_idle())
		|| (phyadapter->antenna_state == TX && frame != NULL))
		return NULL;

	if (!difs_finished)
	{
		if (!energized)
		{
			difs_start(current_time);
			update_gui_timers(current_time);
		}
		else if (txBuffer.abs_re() == dot11ShortRetryLimit + 1)
		{
			retry_offset(-1);
		}
		return NULL;
	}
	else return tx_dequeue(current_time);
}
/* respond with a new frame when this function is called from the channel_evaluate function */
void Station::tx_response(uint current_time, Frame *received_frame, vector<sptrFrame> &cellrow)
{
	std::shared_ptr<Frame> response_frame = NULL;
	auto source = received_frame->getSource();
	auto sequence = received_frame->getSequence();
	auto dest = received_frame->getDest();
	auto datadur = received_frame->get_dat_duration();
	uint mcs = 0, time = 0, seq = -1;

	switch (received_frame->subval())
	{
	case Global::_RTS:
		if (wait_for_timers)
		{
			if (phy_cs_active(current_time) && apevents)
			{
				cts_int_timer->reset();
				ack_int_timer->reset();
			}
			else error_out("Trying to reset int timers at rx of another RTS");
		}
		response_frame = sptrCTS(new CTS(nextslottime_us(current_time + 1 + dot11a_sifs), dest, sequence, source, mcs, datadur));
		time = response_frame->getTime();
		if (response_frame != NULL && time < Global::produration)
			cellrow[time] = response_frame;
		else return;
		handshake[source] = sequence;
		dat_int_reset = false;
		break;
	case Global::_CTS:
		seq = txqueue_seq();
		mcs = maclayer->mcs_map(source);
		response_frame = sptrDATA(new DATA(nextslottime_us(current_time + 1 + dot11a_sifs), dest, sequence, source, mcs, datadur, Global::data_fragments));
		time = response_frame->getTime();
		backoff_timer->success();
		if (!(backoff_timer->slots < 0))
		{
			logger->writeline(num2str(current_time) + " Negating rts backoff (all " + num2str(backoff_timer->slots) + " slots), and resetting BO widow");
			backoff_timer->slots = -1;
		}
		if (response_frame != NULL && time < Global::produration)
			cellrow[time] = response_frame;
		else return;
		cts_received = true;
		logger->writeline(num2str(time) + " DATA release, seq" + num2str(seq != sequence ? sequence : txBuffer.seqn()));

		break;
	case Global::_DATA:
		response_frame = sptrACK(new ACK(nextslottime_us(current_time + 1 + dot11a_sifs), dest, sequence, source, mcs, datadur));
		time = response_frame->getTime();
		if (response_frame != NULL && time < Global::produration)
			cellrow[time] = response_frame;
		else return;
		dat_int_reset = false;
		break;
	case Global::_ACK:
		backoff_timer->success();
		if (!(backoff_timer->slots < 0))
		{
			logger->writeline(num2str(current_time) + " Negating ack backoff (all " + num2str(backoff_timer->slots) + " slots), and resetting BO widow");
			backoff_timer->slots = -1;
		}

		seq = txqueue_seq();
		devent[source][actual_times->at(seq != sequence ? sequence : txBuffer.seqn())] = current_time;
		if (seq != sequence)
		{
			logger->writeline(num2str(current_time) + " seq" + num2str(sequence) + " drop recovered (" + num2str(dropped_count.at(source)) + " already dropped)");
			--dropped_count.at(source);
			dropped_latch.reset();
		}
		else
		{
			if (txBuffer.abs_re() < 0) error_out("buffer empty or forgot to drop the frame");
			txQueue.delivered();
			logger->writeline(num2str(current_time) + " qsize: " + num2str(txQueue.size()) + " at DATA del seq" + num2str(txBuffer.seqn()));
			txBuffer.delivered();
		}
		queue_size[source][current_time] = txQueue.size();
		resetPacketMode();
	}
}
void Station::decode_frame(uint current_time, Frame* rx_frame, vector<sptrFrame>& wireless_channel)
{
	std::uniform_real_distribution<double> distribution(0.0, 1.0);
	phy_indication->rx.end = { true, current_time };
	float SINR = ((int)(lin2dB(phy_indication->signal_ave() / (phy_indication->noisal_ave() + system_noise())) * (double)100.0)) / (double)100.0;
	double PSR = 1 - this->getPhyLayer()->get_PhyRate().getSER(rx_frame->get_mcs_idx(), SINR);
	auto prob = distribution(random_generator);
	bool crc_pass = prob <= PSR;

	if (crc_pass) update_timers(current_time, rx_frame);
	bool expected = crc_pass ? isExpectedFrame(rx_frame) : false;
	if (crc_pass && expected)
	{
		tx_response(current_time, rx_frame, wireless_channel);
#ifdef SHOWGUI
		if (GUISTART <= current_time && current_time <= GUIEND)
			for (uint i = 0; i < rx_frame->getDuration(); ++i)
				guiptr->at(current_time - i).receiver->discarded = false;
#endif
	}
	else
	{
		if (response_waiting(current_time))
		{
			frame_drop_check(current_time);
			cts_ack_wrong_pckt = true;
			resetPacketMode();
		}
		if (!crc_pass) logger->writeline(num2str(current_time) + " packet from " + num2str(rx_frame->getSource())
			+ ": Prob at decode: " + num2str(prob) + ", PSR: " + num2str(PSR) + ", crc fail");
	}
	reset_receiver();
}
transciever_mode Station::getAntennaMode()
{
	return phyadapter->antenna_state;
}
// in Watts
float Station::system_noise()
{
	return this->phyadapter->getAntenna()->getChannel()->get_effective_noise();
}
uint Station::getProp(uint station)
{
	return phyadapter->getAntenna()->getChannel()->getPropDelay(station);
}
Location Station::getLocation()
{
	return currentlocation;
}
/* traffic checker to see if there's anymore frame */
bool Station::trafficPresent()
{
	return rts_idx != UINT_MAX;
}
uint Station::get_next_rts_time()
{
	if (!trafficPresent()) return UINT_MAX;
	return rts_idx != actual_times->size() ? actual_times->at(rts_idx) : UINT_MAX;
}
Indication &Station::get_phy_indication()
{
	return *phy_indication;
}
void Station::sifs_unset()
{
	sifs_running = false;
}
bool Station::sifs()
{
	return sifs_running;
}
bool Station::active()
{
	return get_next_rts_time() != UINT_MAX || txQueue.size();
}
bool Station::frame_drop_check(uint &current_time)
{
	auto retries = txBuffer.abs_re();
	if (!retries)
	{
		logger->writeline(num2str(current_time) + " qsize: " + num2str(txQueue.size()) + " at deq, frame dropped" + ", seq" + num2str(txBuffer.seqn()));
		dropped_latch = { true, txBuffer.seqn() };
		++dropped_count.at(txBuffer.getdest());
		txBuffer.discard();
		txQueue.discard();
		resetPacketMode();
		backoff_timer->retry_exceeded();
		return true;
	}
	return false;
}
void Station::evaluate_channel(uint current_time, vector<vector<sptrFrame>>& wireless_channel)
{
	auto rx_output = phy_indication->rx_state_machine(current_time, wireless_channel);
	if (rx_output.energized && difs_finished) // && antenna->state == RX;
		difs_unset();

#ifdef SHOWGUI
	if (GUISTART <= current_time && current_time <= GUIEND)
	{
		for (auto ginfo : rx_output.guiinfo)
		{
			guiptr->at(current_time).add(new gui_frame_stat_rx{ ginfo.source,
			 ginfo.destination, ginfo.type, ginfo.sequence, ginfo.frag });
		}
	}
#endif

	if (phy_indication->rx.start(true, current_time) && !rts_nav->inactive(current_time)) rts_nav_update(current_time);
	if (rx_output.frame) decode_frame(current_time, rx_output.frame, wireless_channel[this->getID()]);
}
void Station::buffer_ap_stats(string input)
{
	if (ap_mode)
	{
		ap_prints.push_back(input);
	}
}

void Station::unbuffer_ap_stats(Logger* logger)
{
	if (ap_mode)
	{
		for (auto s : ap_prints) logger->writeline(s);
	}
}

float Station::total_data_transferred()
{
	float data = 0;
	for (auto station : dest_addresses)
		data += 8 * get_data_bytes(station) / (float)1e6;
	return data;
}
uint Station::prepare_summary()
{
	/* keep track of the min and max to be used later */
	total_data = total_load = total_events_procc = total_events_dropped = last_event = 0;
	//total_events_queued = trafficgen != NULL ? actual_times->size() : 0;

	per_station_summary.clear();
	combined_qlat.clear();
	combined_qsize.clear();

	for (auto& station : dest_addresses)
	{
		auto data_transferred = 8 * get_data_bytes(station) / (float)1e6;
		auto medium_load = trafficgen != NULL ? trafficgen->getLoad(station) : 0;

		auto queued_eventtime = events_queued[station].size();
		auto queued_proccesed = devent[station].size();
		auto queued_dropped = dropped_count[station];

		/* collect summary data to print at the end of the report */
		per_station_summary.emplace_back(unordered_map<string, double>());
		auto& summary = per_station_summary.back();

		summary["id"] = station;
		summary["mcs"] = maclayer->mcs_map(station);
		summary["data_transfered"] = data_transferred;
		summary["medium_load"] = medium_load;
		summary["total_events_ququed"] = queued_eventtime;
		summary["queued_procc"] = queued_proccesed;
		summary["queued_dropped"] = queued_dropped;

		/* compute this data for overall summary below */
		total_data += data_transferred;
		total_load += medium_load;
		total_events_ququed  += queued_eventtime;
		total_events_procc   += queued_proccesed;
		total_events_dropped += queued_dropped;

		if (devent.find(station) != devent.end() && prev(devent[station].end())->second > last_event)
			last_event = prev(devent[station].end())->second;

		/* for overall printout about the network */
		if (devent.find(station) != devent.end())
		{
			combined_qlat.insert(devent.at(station).begin(), devent.at(station).end());
		}

		/* for overall printout about the network */
		if (queue_size.find(station) != queue_size.end())
		{
			combined_qsize.insert(queue_size.at(station).begin(), queue_size.at(station).end());
		}
	}


	/* buffer the contents to be displayed at the end of the program */
	buffer_ap_stats("AP Overall Summary (name:" + self_name + ")");
	buffer_ap_stats(":: Event-time,Dequeue-time,Delta ::");
	for (auto m : combined_qlat)
	{
		auto num = (m.second - m.first) / 1000.0;
		ave_lat += num;
		if (num > max_lat) max_lat = num;
		buffer_ap_stats(num2str(m.first / 1000.0) + "," + num2str(m.second / 1000.0) + "," + num2str(num));
	}

	buffer_ap_stats("\n\n\n:: Queue Size Profiling Info ::");
	for (auto m : combined_qsize)
	{
		ave_size += m.second;
		if (m.second > max_size) max_size = m.second;
		buffer_ap_stats(num2str(m.first / 1000.0) + ',' + num2str(m.second));
	}

	return last_event;
}

void Station::summrize_sim(Logger* common, float endtime)
{
	overall_summary(logger, endtime);

	/* print out the packet latency history */
	for (auto station : dest_addresses)
	{
		logger->change_file(station, ":: Packet Latency Profiling Info ::");

		logger->writeline("Event-time(ms),Dequeue-time(ms),Delta(ms)");
		if (devent.find(station) != devent.end())
		{
			for (auto& e : devent.at(station))
			{
				logger->writeline(num2str(e.first / 1000.0) + "," + num2str(e.second / 1000.0) + "," + num2str((e.second - e.first) / 1000.0));
			}
		}
		else
		{
			logger->writeline("----------- there were no events to this station yet -------------------------------");
		}
	}

	/*print out the queue size info */
	for (auto station : dest_addresses)
	{
		logger->change_file(station, ":: Queue Size Profiling Info ::");

		logger->writeline("Event-time(ms),Queue-size");
		if (queue_size.find(station) != queue_size.end())
		{
			for (auto& e : queue_size.at(station))
			{
				logger->writeline(num2str(e.first / 1000.0) + ',' + num2str(e.second));
			}
		}
		else
		{
			logger->writeline("----------- there were no events to this station yet -------------------------------");
		}
	}

	logger->change_file(); // reset

	/* write network summary */
	overall_summary(common, endtime);
	queue_status(common);
}

void Station::overall_summary(Logger *logger, float endtime)
{
	/* write this in the common logger and in the AP file */
	if (logger != this->logger || ap_mode == true)
	{
		/* writ the global stats */
		logger->writeline("===================================== STATION " + num2str(uniqueID) + " ====================================\n");
		logger->writeline("Total data (Mb)                 :\t" + double2str(total_data, 2));
		logger->writeline("Station load (Mb/s)             :\t" + double2str(total_load, 2));
		logger->writeline("Station throughput (Mb/s)       :\t" + double2str(last_event == 0 ? 0 : total_data * 1e6 / last_event, 2));
		logger->writeline("Packets queued                  :\t" + num2str(total_events_ququed));
		logger->writeline("Packets transmitted             :\t" + num2str(total_events_procc));
		logger->writeline("Last transmitted (ms)           :\t" + double2str(last_event / 1e3, 2));
		logger->writeline("Simulation duration (ms)        :\t" + double2str(endtime, 2));
		logger->writeline("Packets dropped                 :\t" + num2str(total_events_dropped));
		logger->writeline("Data payload size (bytes)       :\t" + num2str(Global::data_pack_size));

		logger->write(    "MCS Index (dest-id:mcs-number)  :");
		for (auto d : dest_addresses) logger->write("\tstation" + num2str(d) + ":mcs" + num2str(maclayer->mcs_map(d)));
		logger->write("\n");
		logger->writeline("Seed                            :\t" + num2str(trafficgen != NULL ? trafficgen->getseed() : -1));
		logger->write("\n");
		logger->writeline("---------------------- per station summary shown below -----------------------------\n");

		/* writ the per station stats */
		logger->write(    "Destination station             :\t");
		for (auto& station : per_station_summary)
		{
			logger->write(num2str((int)station["id"]) + "\t");
		}
		logger->write("\n");

		logger->write(    "MCS Index                       :\t");
		for (auto& station : per_station_summary)
		{
			logger->write(("[" + num2str((int)station["mcs"])) + "]\t");
		}
		logger->write("\n");

		logger->write(    "Total data (Mb)                 :\t");
		for (auto& station : per_station_summary)
		{
			logger->write(double2str(station["data_transfered"], 2) + "\t");
		}
		logger->write("\n");

		logger->write(    "Medium load (Mb/s)              :\t");
		for (auto& station : per_station_summary)
		{
			logger->write(double2str(station["medium_load"], 2) + "\t");
		}
		logger->write("\n");

		logger->write(    "Packets queued                  :\t");
		for (auto& station : per_station_summary)
		{
			logger->write(num2str((int)station["total_events_ququed"]) + "\t");
		}
		logger->write("\n");

		vector<int> inactive;
		logger->write(    "Packets procesed                :\t");
		for (auto& station : per_station_summary)
		{
			logger->write(num2str((int)station["queued_procc"]) + "\t");
			if (station["queued_procc"] == 0)
				inactive.emplace_back(station["id"]);
		}
		logger->write("\n");

		logger->write(    "Packets dropped                 :\t");
		for (auto& station : per_station_summary)
		{
			logger->write(num2str((int)station["queued_dropped"]) + "\t");
		}
		logger->write("\n");

		/* print out a warning for all the stations that did not receive packets */
		for (auto& station : inactive)
			logger->writeline("WARNING: All events droppped for this destination id " + num2str(station));
	}
}
void Station::queue_status(Logger* logger)
{
	logger->writeline("Average latency                 :\t" + double2str(combined_qlat.empty() ? 0 : ave_lat / combined_qlat.size(), 2));
	logger->writeline("Maximum latency                 :\t" + double2str(max_lat, 2));
	logger->writeline("Average queue size              :\t" + double2str(combined_qsize.empty() ? 0 : ave_size / combined_qsize.size(), 2));
	logger->writeline("Maximum queue size              :\t" + num2str((int)max_size));
	logger->writeline("\n\n");

#ifndef SHOWGUI
	if (Global::DEBUG_END > 50000
		&& total_events_ququed != (total_events_procc + total_events_dropped))
		error_out("NOT DONE YET. PACKETS LEFT IN THE QUEUE.");
#endif
}
