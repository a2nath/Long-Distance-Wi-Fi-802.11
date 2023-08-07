#pragma once
#define _USE_MATH_DEFINES
#include <random>
#include <unordered_map>
#include <map>
#include <memory>
#include <limits>
#include <climits>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include "Links.h"
#include "Logger.h"

/* possible macros that can be enabled from here or in the compile script
*
*
#define DETERMINISTIC             // use the same seed between simulations for rng
#define SHOWGUI                   // show the visualization if API is available
#define SHOWOUT                   // more verbose output
#define REDUNDANT_RETRIES         // retry more often
#define GUISTART            0     // start time of the GUI render
#define GUIEND              5000  // end time   of the GUI render
*
*
---------------------------*/

/* windows specific code  */
#ifdef _WIN32

#include <Windows.h>
#define error_out( s )                                     \
{                                                          \
	logs.done(" with error ");				               \
	std::wostringstream os_;                               \
	os_ << s << "\n";                                      \
	OutputDebugStringW( os_.str().c_str() );	           \
	throw;	                                               \
}
#define debugout( p )                                      \
{                                                          \
	std::wostringstream os_;                               \
	os_ << p;                                              \
	OutputDebugStringW( os_.str().c_str() );	           \
}

/* linux specific code */
#else
#include <unistd.h>
#include <string.h>
#define error_out( s )                                    \
{                                                         \
	stringstream oss;                                     \
	cerr.rdbuf( oss.rdbuf() );                            \
	logs.done(" with error. Error " + oss.str());         \
	throw;                                                \
}
#define debugout( p )                                     \
{                                                         \
	stringstream oss;                                     \
	streambuf* old = cerr.rdbuf( oss.rdbuf() );           \
	cerr << " " << p;                                     \
	cout << oss.str();                                    \
	cerr.rdbuf( old );                                    \
}

#endif


extern std::string input_dir;
extern std::string output_dir;
extern std::string per_rate_file;
extern std::string error_rate_file;
extern std::string in_mapping_file;
extern std::string in_distance_map;
extern std::string in_pathloss_map;
extern std::string in_simulation_params;

namespace IO
{
	using namespace std;
	void read_packet_error_rate_lut();
	void read_symbol_error_rate_lut();
	vector<string> readfile(string filename, vector<vector<string>>& out, bool = false);
}

typedef unsigned int uint;
typedef uint rts_release_time;
typedef uint rts_queued_time;
typedef uint frame_type;
typedef uint frame_sequence_num;
typedef int retry_count;
typedef uint station_number;
typedef uint mcs_index;
typedef uint data_rate;
typedef uint prop_del_us;
typedef std::string station_name;

typedef std::vector<uint> uivector;
typedef std::vector<double> doublevector;
typedef std::vector<std::vector<float>> floattable;

enum enm {
	conns = 0,
	/* Medium load in megabits per second */
	payload_rate = 1,
	/* TCP or UDP */
	contype = 2,
	progdur = 3,
	simdur = 4,
	/* size of data payload in bytes */
	datbytes = 5,
	fquency = 6,
	bwidth = 7,
	dsegments = 8,
	atout = 9,
	antpower = 10,
	stanames = 11,
	debugend = 12,
	/* distance scale at which to operate the wifi, 0 or > 1 means tradional wifi */
	pfactor = 13,
	/* backoff window length */
	chwdow = 14,
	/* random number seed */
	seed = 15,
	/* backoff window upper bound */
	acwmax = 16,
	/* backoff window lower bound */
	acwmin = 17,
	/* retry limit */
	relim = 18
};
/*
IDLE is for the GUI package.
Not used for the Simulation package.
*/
enum transciever_mode { IDLE = 0b00, TX = 0b10, RX = 0b01 };
const float lightspeed = 3e8;

template<typename T, typename U>
using umap = std::unordered_map<T, U>;
template <class U> float sum(std::vector<U> &list)
{
	double sum = 0;
	for (auto &s : list)
		sum += s;
	return sum;
}
template <class U> float ave(std::vector<U> &list)
{

	return sum(list) / (float)list.size();
}

/* Logging Objects */
struct Global {
	static umap<uint, std::string> mcs_vs_modname;
	static umap<std::string, double> data_bits_per_OFDM_symbol;
	static umap<std::string, float> PER_thrsh;
	static umap<std::string, umap<uint, double>> data_rates;
	static umap<std::string, std::map<float, double>> SER_TABLE;
	static umap<std::string, umap<uint, float>> receive_busy_threshold;
	static Link connections;
	static uint station_count;
	static uint data_pack_size;
	static uint data_fragments;
	static uint channel_number;
	static uint traffic_type;
	static std::vector<unsigned long int> seeds;
	// in Watts
	static std::vector<float> txpowers;
	static std::multimap<station_number, float> traffic_load;
	static std::string ap_station;
	static float bandwidth;
	static float frequency;
	static uint produration;
	static uint simduration;
	static bool adapt_int_tout;
	static std::map<std::string, uint> sta_name_map;
	static float prop_factor;
	static uint DEBUG_END;
	static uint chwindow;
	static uint aCWmax;
	static uint aCWmin;
	static uint dot11ShortRetryLimit;
	enum frame_map { _DATA = 0b0000, _RTS = 0b1011, _CTS = 0b1100, _ACK = 0b1101 };
};
struct Logs {
	vector<Logger*> stations;
	Logger *common;

	void done(string s = "")
	{
		auto name = common->getname();
		for (auto &sta : stations)
		{
			sta->writeline("\n\n\nOverall simulation duration " + s + " ms");
			delete sta;
		}

		delete common;

#ifdef _WIN32
		system(("C:\\Program\ Files\\Sublime\ Text\\sublime_text.exe " + name).c_str());
#else
		cout << "Logs saved in directory " << system(string("dirname " + name + "").c_str()) << endl;
#endif
	}
};
extern Logs logs;

/* return string with a requested precision for the input*/
inline std::string double2str(double num, std::size_t const p) {
	std::stringstream sstrm;
	sstrm << std::setprecision(p) << std::fixed << num;

	return sstrm.str();
}

/* Global helper functions */
template<class T> std::string num2str(const T &input) { return std::to_string(input); }

template<class T>
std::string num2str(const umap<uint, T>& input)
{
	string output = "";
	for (auto m : input)
		output += std::to_string(m.second) + ", ";
	output.pop_back();
	return output;
}

template<class T>
std::string num2str(const vector<T>& input)
{
	string output = "";
	for (auto m : input)
		output += std::to_string(m) + ",";
	output.pop_back();
	return output;
}

template<class T> T str2num(string &s)
{
	T x = 0;
	std::stringstream parser(s);
	parser >> x;
	return x;
};

inline int create_dir(std::string path)
{
	auto ret = mkdir(path.c_str());
	if (ret == -1)
	{
		if (errno != EEXIST)
		{
			throw std::runtime_error("Cannot create a new directory " + path + ". " + strerror(errno));
		}
	}
	return 0;
}
int get_sinr2idx(float sinr);
double get_per(mcs_index mcs, float sinr);
double get_per(string mod_scheme, float sinr);
double mcs2bps(uint mcs);
double mcs2thru(uint mcs);
uint data_size(uint mpdu_info, mcs_index mcs);
uivector linspace(double a, double b, int n);
double phyStr2Mbps(std::string name);
double dBm2W(double dBm);
double W2dBm(double W);
double dB2lin(double dB);
double lin2dB(double dB);
float micro2s(double s);
uint s2micro(double s);
/* Converts from duration in usecond to slot-time unit
Inputs: float duration in usecond, boolean if want to round it down
Output: returns the slot number */
uint dur2slots(uint duration, bool round_down = 0);
uint slots2dur(uint slots);
uint nextslottime_us(uint duration);
uivector nextslottime_us(const uivector &input);
void dout(std::string message, bool error = 0);

/* Physical layer parameters */
// in dB
const float G_ap = 12.0;
// in dB
const float G_cl = 2.0;
// in MHz
const uint frequency = 530;
// in dB
const float system_noise_figure = 5.00;
// SER table Min SNR
const float snr_min = -10.0;
// SER table Max SNR
const float snr_max = 40.0;

/* .11a constants  in microseconds */
const uint dot11a_sifs = 16;
const uint dot11a_difs = 34;
const uint dot11a_eifs = 43;
const uint dot11a_slot_time = 9;
const uint symbolrate = 250000;
const uint aPHY_RX_START_Delay = 25;
const uint aPreambleLength = 16;
const uint aPLCPHeaderLength = 4;
const uint aMPDUMaxLength = 4095;
const uint aCCATime = 3;
const uint CTSTimeoutInterval = dot11a_sifs + dot11a_slot_time + aPHY_RX_START_Delay;
const uint DATTimeoutInterval = dot11a_sifs + 38 * dot11a_slot_time;
const uint ACKTimeoutInterval = dot11a_sifs + dot11a_slot_time + aPHY_RX_START_Delay;
const uint rts_nav_timeout_interval = 2 * dot11a_sifs + aPHY_RX_START_Delay + 2 * dot11a_slot_time;
const uint max_mcs_count = 8;

/* in number of slots for backoff window limits */
/* backoff algo: choose random num between [0, CW], where aCWmin < CW < aCWmax */
// in dBm
const float thermal_per_bandwidth = -174.0;

// in dBm
const float dot11_CCA_threshold = -82.0;

const uint RTSCTSACK_DUR = 5;
const uint DATA_DUR = 5;

//RTS threshold, to tell a station if RTS/CTS is required given the length of a short data frame, because handshaking is costly
//can be never, always, or if the size is above a certain threshold
const uint dot11RTSThreshold = 3000;
//retry mechanism short

// frame related
const uint service_tail = 22; //bits
const uint preamble_signal = 20; //us
