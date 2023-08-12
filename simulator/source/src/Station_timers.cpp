#include "../inc/Station.h"

/* ---------------------- STATION TIMERS ---------------------- */
void Station::update_timers(uint current_time, Frame *received_frame)
{
	auto sta = this->uniqueID;
	simtime = current_time;
	set_wait(STAspace::BUSY);
	int source = phy_indication->rx.end(true, current_time) || phy_indication->tx.end(true, current_time) ? received_frame->getSource() : -1;
	int type = phy_indication->rx.end(true, current_time) || phy_indication->tx.end(true, current_time) ? received_frame->subval() : -1;
	int destin = phy_indication->rx.end(true, current_time) || phy_indication->tx.end(true, current_time) ? received_frame->getDest() : -1;
	int seqnum = phy_indication->rx.end(true, current_time) || phy_indication->tx.end(true, current_time) ? received_frame->getSequence() : -1;

	/* difs section */
	if (difs_wait->running(current_time) && difs_wait->expired(current_time))
	{
		if (txBuffer())
		{
			difs_set(STAspace::DONE);
			difs_wait->reset();
			if (is_DIFS_busy() || !(backoff_timer->slots < 0))
			{
				if (backoff_timer->slots < 0)
				{
					backoff_start(current_time);
				}
				else backoff_resume(current_time);
			}
			else
			{
				auto frame = txBuffer.get(actual_times.time(txBuffer.seqn()));
				if (frame != NULL && frame->getTime() == current_time - dot11a_difs)
				{
					txBuffer.buf.first = dot11ShortRetryLimit + 1;
					resetPacketMode();
					txBuffer.settime(current_time);
					set_wait(STAspace::IDLE);
				}
				else backoff_start(current_time);
			}
			set_DIFS_busy(current_time, false);
		}
		else dout(num2str(current_time) + ": sta(" + num2str(sta) + ")" + " buffer empty or retry at 0 before difs had started?", true);
	}
	else if (!difs_wait->expired(current_time))
	{
	}
	else
	{
		/* cts + rts interval timeout section [Program call] */
		if (source == this->uniqueID)
		{
			if (!cts_int_timer->running() && type == Global::_RTS)
				cts_int_start(current_time + (apevents ? rrt_map.at(txBuffer.getdest()) : 0));
			if (!dat_int_timer->running() && type == Global::_CTS)
				dat_int_start(current_time);
			else if (!ack_int_timer->running() && type == Global::_DATA)
				ack_int_start(current_time + (apevents ? rrt_map.at(txBuffer.getdest()) : 0));
		}
		/* NAV virtual timers + Backoff section [Station call] */
		else if (type == Global::_RTS && destin != this->uniqueID)
		{
			rts_nav_start(current_time, received_frame);
		}
		else if (type == Global::_CTS && destin != this->uniqueID)
		{
			cts_nav_start(current_time, received_frame);
		}
		else if (!backoff_timer->running(current_time) && dat_int_timer->running() && phy_indication->rx.start.time <= current_time)
		{
			set_wait(STAspace::IDLE);
			dat_int_timer->reset();
			difs_unset();
			dat_int_reset = true;
		}
		else if (!backoff_timer->running(current_time) && (dat_int_reset || cts_int_timer->running() || dat_int_timer->running() || ack_int_timer->running()))
		{
			if (dat_int_reset)
			{
				set_wait(STAspace::IDLE);
				if (!energized)
				{
					frame_drop_check(current_time);
					resetPacketMode();
					difs_start(current_time);
					dat_int_reset = false;
				}
			}
			else
			{
				bool is_cts_timer = cts_int_timer->running() ? true : false;
				bool is_dat_timer = dat_int_timer->running() ? true : false;
				Timer* int_timer = is_cts_timer ? cts_int_timer.get() : (is_dat_timer ? dat_int_timer.get() : ack_int_timer.get());

				if (int_timer->expired(current_time))
				{
					set_wait(STAspace::IDLE);
					int_timer->reset();
					difs_unset();

					if (!energized)
					{
						frame_drop_check(current_time);
						resetPacketMode();
						difs_and_backoff(current_time);
					}
				}
			}
		}
		else if (virtual_cs_active(current_time))
		{
#ifndef nav_interruptable
			logger->write(destin == this->uniqueID && type == Global::_RTS ?
				(num2str(current_time) + ": Could not honor the RTS frame\n") : "");
#else
			if (destin == this->uniqueID && type == Global::_RTS) //station prioritizes its own packets over receiving them
			{
				response_waiting(current_time);
				set_wait(STAspace::IDLE);
				resetPacketMode();
				rts_nav->reset();
				cts_nav->reset();
			}
#endif
		}
		else set_wait(STAspace::IDLE);

		// GUI-wise this is the period between starting the backoff and the next slot boundary where the backoff visually starts.
		if (!(backoff_timer->running(current_time) || backoff_timer->expired(current_time)))
		{
			if (destin == this->uniqueID && (type == Global::_CTS || type == Global::_ACK))
			{
				if (!exp_cts_ack_resp(type)) error_out("Not the expected frame");
				backoff_timer->slots = -1;
				backoff_reset(current_time);
			}
			set_wait(STAspace::BUSY);
		}
		else if (backoff_timer->running(current_time))
		{
			// timer may or may not be expired.
			backoff_timer->update(current_time);
			difs_finished = true;
			if (backoff_timer->expired(current_time))
			{
				backoff_reset(current_time);
				if (!backoff_timer->slots)
				{
					if (!energized)
					{
						backoff_timer->slots = -1;
						if (txBuffer())
						{
							resetPacketMode();
							txBuffer.settime(current_time);
						}
						else error_out("Why is this the case?");
					}
				}
				else if (!energized) dout(num2str(current_time) + " This problem again", true);
			}
			else set_wait(STAspace::BUSY);
		}
		else if (phy_indication->rx.end.b == true)
		{
			if (destin == this->uniqueID && type == Global::_RTS)
			{
				if (response_waiting(current_time))
					frame_drop_check(current_time);
				resetPacketMode();
			}
		}
	}

	if (rts_nav->running(current_time))
	{
		if (rts_nav->expired(current_time))
		{
			rts_nav->reset();
			if (phy_indication->rx.start.time > current_time && !energized)
			{
				difs_start(current_time);
			}
		}
		else
		{
			if (phy_indication->rx.start(true, current_time))
			{
				cout << "Works!";
				rts_nav_update(current_time);
			}
		}
	}

	if (cts_nav->running(current_time) && cts_nav->expired(current_time))
	{
		cts_nav->reset();
		if (phy_indication->rx.start.time > current_time && !energized)
		{
			difs_start(current_time);
		}
	}

	queue2buffer(current_time);
	update_gui_timers(current_time);
}
/* difs functions */
bool Station::is_DIFS_busy()
{
	return static_cast<dot11_difs*>(difs_wait.get())->medium_busy;
}
void Station::set_DIFS_busy(uint current_time, flag state)
{
	static_cast<dot11_difs*>(difs_wait.get())->medium_busy = state;
}
bool Station::is_DIFS_expired(uint current_time)
{
	return difs_wait->expired(current_time);
}
bool Station::is_backoff_expired(uint current_time)
{
	return backoff_timer->expired(current_time);
}
// difs reset when finished transmitting or finished receiving
void Station::difs_unset()
{
	difs_set(STAspace::UNFIN);
}
void Station::difs_set(flag flag)
{
	difs_finished = flag;
}
void Station::difs_reset(uint current_time, flag setstate)
{
	logger->write(num2str(current_time) + " difs cancelled, last start-time " + num2str(difs_wait->start_time));
	static_cast<dot11_difs*>(difs_wait.get())->medium_busy = false;
	difs_wait->reset();
	if (!setstate) set_wait(STAspace::IDLE);
}
void Station::difs_start(uint current_time)
{
	if (!txQueue.size() || !backoff_timer->inactive(current_time)) return;
	for (auto &timer : { rts_nav.get(), cts_nav.get(), cts_int_timer.get(), dat_int_timer.get(), ack_int_timer.get(), difs_wait.get() })
	{
		if (!timer->inactive(current_time)) return;
	}
	set_wait(STAspace::BUSY);
	difs_wait->start(current_time);
	logger->writeline(num2str(difs_wait->start_time) + " difs start until " + num2str(difs_wait->time()));
}

/* backoff start functions */
void Station::backoff_resume(uint current_time)
{
	if (!virtual_cs_active(current_time) && difs_wait->inactive(current_time))
	{
		backoff_timer->resume(current_time);
		if (backoff_timer->slots)
		{
			set_wait(STAspace::BUSY);
		}
		else if (current_time == nextslottime_us(current_time))
		{
			set_wait(STAspace::IDLE);
			if (!energized)
			{
				backoff_timer->slots = -1;
				if (txBuffer())
				{
					resetPacketMode();
					txBuffer.settime(current_time);
				}
			}
			backoff_reset(current_time);
			return;
		}
	}
}
void Station::backoff_start(uint current_time)
{
	if (!virtual_cs_active(current_time) && difs_wait->inactive(current_time))
	{
		backoff_timer->start(current_time);
		if (backoff_timer->slots)
		{
			set_wait(STAspace::BUSY);
		}
		else if (current_time == nextslottime_us(current_time))
		{
			set_wait(STAspace::IDLE);
			if (!energized)
			{
				backoff_timer->slots = -1;
				if (txBuffer())
				{
					resetPacketMode();
					txBuffer.settime(current_time);
				}
			}
			logger->write(cts_ack_wrong_pckt ? ", generated from wrong packet after RTS/DATA\n" : "\n");
			backoff_reset(current_time);
			return;
		}

		logger->write(cts_ack_wrong_pckt ? ", generated from wrong packet after RTS/DATA\n" : "\n");
	}
}
void Station::make_BCKOFF_busy(uint current_time)
{
	if (!backoff_timer->inactive(current_time))
		backoff_timer->make_boff_busy(current_time);
}
void Station::difs_and_backoff(uint current_time)
{
	difs_start(current_time);
	if (backoff_timer->slots < 0 && !difs_wait->expired(current_time))
	{
		logger->write(num2str(current_time) + " ");
		backoff_timer->slots = backoff_timer->getslots(current_time);
		logger->write(", generated from timeout of CTS/ACK\n");
	}
}
void Station::backoff_reset(uint current_time)
{
	logger->writeline(num2str(current_time) + " backoff expired");
	backoff_timer->reset();
	cts_ack_wrong_pckt = false;
}
void Station::set_wait(flag flag)
{
	wait_for_timers = flag;
}
bool Station::is_cts_expired(uint current_time)
{
	return cts_int_timer->expired(current_time);
}
bool Station::is_dat_expired(uint current_time)
{
	return dat_int_timer->expired(current_time);
}
bool Station::is_ack_expired(uint current_time)
{
	return ack_int_timer->expired(current_time);
}
bool Station::is_nav_rts_expired(uint current_time)
{
	return rts_nav->expired(current_time);
}
bool Station::is_nav_cts_expired(uint current_time)
{
	return cts_nav->expired(current_time);
}
/* Timeout intervals */
void Station::cts_int_start(uint current_time)
{
	set_wait(STAspace::BUSY);
	cts_int_timer->start(current_time);
}
void Station::dat_int_start(uint current_time)
{
	set_wait(STAspace::BUSY);
	dat_int_timer->start(current_time);
}
void Station::ack_int_start(uint current_time)
{
	set_wait(STAspace::BUSY);
	ack_int_timer->start(current_time);
}
/* Interval timeout active at this time */
bool Station::phy_cs_active(uint current_time)
{
	for (auto &intout : { cts_int_timer.get(), dat_int_timer.get(), ack_int_timer.get() })
		if (!intout->inactive(current_time)) return true;
	return false;
}
/* NAV timers */
void Station::rts_nav_start(uint current_time, Frame* frame)
{
	set_wait(STAspace::BUSY);
	rts_nav->start(current_time, frame);
	if (!cts_nav->inactive(current_time))
		static_cast<tRTSNAV*>(cts_nav.get())->nav_update(rts_nav->end_time);
}
void Station::rts_nav_update(uint current_time)
{
	static_cast<tRTSNAV*>(rts_nav.get())->tout_update(current_time);
}
void Station::cts_nav_start(uint current_time, Frame* frame)
{
	set_wait(STAspace::BUSY);
	cts_nav->start(current_time, frame);
	if (!rts_nav->inactive(current_time))
		static_cast<tRTSNAV*>(rts_nav.get())->nav_update(cts_nav->end_time);
}
bool Station::virtual_cs_active(uint current_time)
{
	for (auto &nav : { rts_nav.get(), cts_nav.get() })
		if (!nav->inactive(current_time)) return true;
	return false;
}
/* ----------------------  GUI ---------------------- */
void Station::update_gui_timers(uint current_time)
{

#ifdef SHOWGUI
	if (!(GUISTART <= current_time && current_time <= GUIEND)))
		return;

	auto timer_state = guiptr->at(current_time).getmode();
	auto timerend = guiptr->at(current_time).gettimer();
	if (!is_DIFS_expired(current_time) && difs_wait->running(current_time))
	{
		*timer_state = difs;
		*timerend = difs_wait->end_time;
	}
	else if (!is_cts_expired(current_time) && cts_int_timer->running(current_time))
	{
		*timer_state = cts_timeout;
		*timerend = cts_int_timer->end_time;
	}
	else if (!is_dat_expired(current_time) && dat_int_timer->running(current_time))
	{
		*timer_state = dat_timeout;
		*timerend = dat_int_timer->end_time;
	}
	else if (!is_ack_expired(current_time) && ack_int_timer->running(current_time))
	{
		*timer_state = ack_timeout;
		*timerend = ack_int_timer->end_time;
	}

	if (!is_nav_rts_expired(current_time) && rts_nav->running(current_time))
	{
		*timer_state = nav_rts;
		*timerend = rts_nav->end_time;
	}

	if (!is_nav_cts_expired(current_time) && cts_nav->running(current_time))
	{
		*timer_state = nav_cts;
		*timerend = cts_nav->end_time;
	}

	if (!is_backoff_expired(current_time) && backoff_timer->running(current_time))
	{
		*timer_state = backoff;
		*timerend = backoff_timer->end_time;
	}

#endif
	return;
}
