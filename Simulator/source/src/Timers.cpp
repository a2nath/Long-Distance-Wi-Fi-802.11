#include "../inc/timers.h"

/* Backoff Core Mechanism */
Backoff::Backoff(uint station_id, std::mt19937_64 &gen_object) : sta(station_id), generator(gen_object)
{
	//seed = std::chrono::system_clock::now().time_since_epoch().count();
	uint min = log2(Global::aCWmin), max = log2(Global::aCWmax);
	for (uint i = min; i < max + 1; i++)
		max_windows.push_back(pow(2, i) - 1);
	p_window = &max_windows[0];
}
void Backoff::success()
{
	p_window = &max_windows[0];
}
void Backoff::fail()
{
#ifdef REDUNDANT_RETRIES
	if (*p_window != max_windows.back())
		++p_window;
	else
		cout << "";
#else
	++p_window;
#endif
}
uint Backoff::gen_time(uint time)
{
	auto &log = logs.stations[sta];
	uint upper_bound;
	if (*p_window != max_windows.back())
		upper_bound = *p_window;
	else
		upper_bound = max_windows.back();

	if (upper_bound > Global::aCWmax)
		dout("Backoff Error, window exceeded: " + num2str(upper_bound) + " sta: " + num2str(sta), true);
	std::uniform_int_distribution<int> distribution(0, upper_bound);
	uint slot_count = round(distribution(generator));
	dout(num2str(time) + " " + num2str(sta)  + ": Slot count " + num2str(slot_count) + "/" + num2str(*p_window));
	log->write("Slot count : " + num2str(slot_count) + "/" + num2str(*p_window));
	fail();
	return slot_count;
}


/* Backoff Manager functions */
Backoff_Manager::Backoff_Manager(uint station_id, std::mt19937_64 &generator_object) : id(station_id),
generator(generator_object), slots(-1), start_time(OFF), end_time(OFF), slotmarked({ 0,false })
{
	botimer = new Backoff(station_id, generator);
}
Backoff_Manager::~Backoff_Manager()
{
	delete botimer;
}
void Backoff_Manager::retry_exceeded()
{
	success();
}
void Backoff_Manager::success()
{
	botimer->success();
}
bool Backoff_Manager::running(uint current_time)
{
	return start_time <= current_time && start_time != OFF ? true : false;
}
void Backoff_Manager::reset()
{
	start_time = OFF;
	end_time = OFF;
}
void Backoff_Manager::update(uint time)
{
	auto slot_after_first = dur2slots(start_time + 1);
	auto current_slot = dur2slots(time, true);
	if (slotmarked.slot != current_slot && current_slot >= slot_after_first)
	{
		if (slotmarked.suspend == false)
		{
			slotmarked.slot = current_slot;
			--slots;
		}
		else // medium is busy. suspend yourself peasant!
		{
			slotmarked.suspend = false;
			reset();
		}
	}
}
void Backoff_Manager::make_boff_busy(uint time)
{
	auto s = slotsystem{ dur2slots(time, true), true };
	if ((slotmarked.slot == s.slot && slotmarked.suspend == s.suspend) || start_time == end_time) return;
	slotmarked = s;
}
int Backoff_Manager::time()
{
	return end_time;
}
bool Backoff_Manager::expired(int current_time)
{
	return current_time >= end_time;
}
void Backoff_Manager::resume(uint time)
{
	if (!expired(time))
		error_out("BO timer is not expired");
	dout(num2str(time) + " " + num2str(id) + ":Trying to resume BO");

	start_time = nextslottime_us(time);
	end_time = start_time + slots2dur(slots);
	logs.stations[id]->writeline(num2str(time) + " " + num2str(id) + ":Old slot count: " + num2str(slots) + ", start-time " + num2str(start_time));
	dout(num2str(time) + " " + num2str(id) + ":BO started, slots set to: " + num2str(slots) + ", end time: " + num2str(end_time) + ", resumed");
}
void Backoff_Manager::start(uint time)
{
	if (!expired(time))
		error_out("BO timer is not expired");

	slots = getslots(time);
	start_time = nextslottime_us(time);
	end_time = start_time + slots2dur(slots);
	logs.stations[id]->write(", start-time " + num2str(start_time));
	dout(num2str(time) + " " + num2str(id) + ":BO started, slots set to: " + num2str(slots) + ", end time: " + num2str(end_time) + ", started ");
}
uint Backoff_Manager::getslots(uint time)
{
	return botimer->gen_time(time);
}
bool Backoff_Manager::inactive(int current_time)
{
	return (start_time == end_time == OFF) || (expired(current_time) && !running(current_time));
}