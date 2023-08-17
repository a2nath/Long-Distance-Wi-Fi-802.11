#include <math.h>
#include <iostream>
#include <cassert>
#include <functional>
#include "json/json.h"
#include "../inc/common.h"
#define _USE_MATH_DEFINES

using namespace std;

std::string input_dir = "inputs/";
std::string output_dir = "results/";
std::string per_rate_file = "lut_per_threshold_table.txt";
std::string error_rate_file = "lut_error_rate_table.txt";
std::string in_mapping_file = "splat_station_bindings.txt";
std::string in_distance_map = "splat_distance_table.txt";
std::string in_pathloss_map = "splat_pathloss_table.txt";
std::string in_simulation_params = "simulation_parameters.json";

umap<uint, std::string> Global::mcs_vs_modname = {
	{ 0, "BPSK12" },
	{ 1, "BPSK34" },
	{ 2, "QPSK12" },
	{ 3, "QPSK34" },
	{ 4, "16QAM12" },
	{ 5, "16QAM34" },
	{ 6, "64QAM23" },
	{ 7, "64QAM34" }
};

umap<std::string, double> Global::data_bits_per_OFDM_symbol = {
	{ "BPSK12", 24 },
	{ "BPSK34", 36 },
	{ "QPSK12", 48 },
	{ "QPSK34", 72 },
	{ "16QAM12", 96 },
	{ "16QAM34", 144 },
	{ "64QAM23", 192 },
	{ "64QAM34", 216 }
};

umap<std::string, float> Global::PER_thrsh;

umap<std::string, umap<uint, double>> Global::data_rates = {
	{ "BPSK12",{ { (uint)5e6,(uint)1.5e6 },{ (uint)10e6,(uint)3e6 },{ (uint)20e6,(uint)6e6 } } },
	{ "BPSK34",{ { (uint)5e6,(uint)2.25e6 },{ (uint)10e6,(uint)4.5e6 },{ (uint)20e6,(uint)9e6 } } },
	{ "QPSK12",{ { (uint)5e6,(uint)3e6 },{ (uint)10e6,(uint)6e6 },{ (uint)20e6,(uint)12e6 } } },
	{ "QPSK34",{ { (uint)5e6,(uint)4.5e6 },{ (uint)10e6,(uint)9e6 },{ (uint)20e6,(uint)18e6 } } },
	{ "16QAM12",{ { (uint)5e6,(uint)6e6 },{ (uint)10e6,(uint)12e6 },{ (uint)20e6,(uint)24e6 } } },
	{ "16QAM34",{ { (uint)5e6,(uint)9e6 },{ (uint)10e6,(uint)18e6 },{ (uint)20e6,(uint)36e6 } } },
	{ "64QAM23",{ { (uint)5e6,(uint)12e6 },{ (uint)10e6,(uint)24e6 },{ (uint)20e6,(uint)48e6 } } },
	{ "64QAM34",{ { (uint)5e6,(uint)13.5e6 },{ (uint)10e6,(uint)27e6 },{ (uint)20e6,(uint)54e6 } } }
};

umap<std::string, std::map<float, double>> Global::SER_TABLE;

std::map<station_name, station_number> Global::sta_name_map;
Link Global::connections;
uint Global::station_count;
uint Global::data_pack_size; // bytes
uint Global::data_fragments;
uint Global::channel_number;
uint Global::traffic_type;
std::vector<unsigned long int> Global::seeds;
std::vector<float> Global::txpowers;
float Global::bandwidth;
float Global::frequency;
uint Global::produration;
uint Global::simduration;
bool Global::adapt_int_tout;
float Global::prop_factor;
uint Global::chwindow;
uint Global::aCWmax;
uint Global::aCWmin;
uint Global::dot11ShortRetryLimit = 0;
uint Global::DEBUG_END;
string Global::ap_station = "big_m";

/*
inputs: mcs number, sinr(dB)
output: per(0...1)
*/
int get_sinr2idx(float sinr)
{
	for (int mcs = max_mcs_count - 1; mcs > -1; --mcs)
	{
		auto sinr_min = Global::PER_thrsh.at(Global::mcs_vs_modname.at(mcs));
		if (sinr >= sinr_min)
		{
			return mcs;
		}
	}
	return -1;
}
uint data_size(uint mpdu_info, mcs_index mcs)
{
	auto mpdu_size = mpdu_info * 8;
	auto dat_size = 1e6 * (ceil((mpdu_size + service_tail) / mcs2bps(mcs))*mcs2bps(mcs)) / mcs2thru(mcs);
	return dat_size + preamble_signal;
}
double get_per(mcs_index mcs, float sinr)
{
	return get_per(Global::mcs_vs_modname.at(mcs), sinr);
}
double get_per(std::string mod_scheme, float sinr)
{
	double retval = -1.0;
	auto table = Global::SER_TABLE.at(mod_scheme);
	if (sinr > 21)
		retval = 0.0;
	else if (sinr < 0)
		retval = 1.0;
	else
	{
		std::map<float, double>::iterator low;
		low = table.lower_bound(sinr);
		if (low == table.end())
		{
			error_out("Could not find PER");
		}
		else if (low == table.begin())
		{
			retval = low->second;
		}
		else
		{
			auto prev = low;
			--prev;
			retval = ((sinr - prev->first) < (low->first - sinr)) ? prev->second : low->second;
		}
	}
	return retval > 1.0 ? 1.0 : retval;
}

double mcs2bps(uint mcs)
{
	return Global::data_bits_per_OFDM_symbol.at(Global::mcs_vs_modname.at(mcs));
}

double mcs2thru(uint mcs)
{
	return Global::data_rates.at(Global::mcs_vs_modname.at(mcs)).at(Global::bandwidth);
}
uivector linspace(double a, double b, int n)
{
	std::vector<uint> array;
	double step = (b - a) / (n - 1);

	while (a <= b) {
		array.push_back(a);
		a += step;           // could recode to better handle rounding errors
	}
	return array;
}

double phyStr2Mbps(string name)
{
	return Global::data_rates.at(name).at(Global::bandwidth);
}
double dBm2W(double dBm)
{
	return dB2lin(dBm - 30);
}
double W2dBm(double W)
{
	return lin2dB(W) + 30.0;
}
double lin2dB(double W)
{
	return 10 * log10(W);
}
double dB2lin(double dB)
{
	return pow(10.0, dB / 10.0);
}
float micro2s(double d)
{
	return d / (double)1000000.0;
}
uint s2micro(double s)
{
	return 1000000 * s;
}
uint slots2dur(uint slots)
{
	return slots * dot11a_slot_time;
}
uint dur2slots(uint duration, bool round_down)
{
	return !round_down ? ceil(duration / (double)dot11a_slot_time) : floor(duration / (double)dot11a_slot_time);
}
uint nextslottime_us(uint duration)
{
	return ceil(duration / (double)dot11a_slot_time)*dot11a_slot_time;
}
uivector nextslottime_us(const uivector &times)
{
	uivector return_vect;
	for (auto &s : times)
	{
		return_vect.push_back(dur2slots(s)*dot11a_slot_time);
	}
	return return_vect;
}

/* this function takes a string and passes it to the macro */
void dout(string message, bool throw_error)
{
#ifdef SHOWOUT
	if (throw_error == false)
	{
		debugout(string(message + "\n").c_str());
	}
#endif
	if (throw_error == true)
	{
		error_out(string(message + "\n").c_str());
	}

}


/* IO functions to take inputs */
namespace IO
{
	using namespace std;

	inline void getstream(string filename, vector<string>& out)
	{
		FILE* fp = fopen(filename.c_str(), "r");

		if (fp == NULL)
			dout("File could not be opened.", true);

#ifdef _WIN32
		const int CHARCOUNT = 16384;
		char line[CHARCOUNT];
		while (fgets(line, CHARCOUNT, fp) != NULL)
#else
#include <stdio.h>
#include <stdlib.h>
		char* line = NULL;
		size_t len = SIZE_MAX;
		while ((getline(&line, &len, fp)) != -1)
#endif
		{
			out.emplace_back(line);

			if (out.back()[0] == '#' || out.back()[0] == '\n' || out.back()[0] == '\r\n') // linux/windows empty line
				out.erase(out.end() - 1);
		}
#ifndef _WIN32
		if (line)
			free(line);
#endif

		fclose(fp);

		if (out.empty())
			throw runtime_error("Error: File is empty or not accessible");
	}

	/* process packet error rate lut */
	void read_packet_error_rate_lut()
	{
		vector<string> file_contents, csvline;

		// Read the input file for simulation parameters
		getstream(input_dir + per_rate_file, file_contents);

		string encoding_scheme, num;
		for (auto& line : file_contents)
		{
			size_t idx;
			while ((idx = line.find_first_of(" \t\n\r\n")) != string::npos) { line.erase(idx); }

			idx = line.find(',');

			assert(0 < idx && idx < line.size());

			try
			{
				Global::PER_thrsh.emplace(line.substr(0, idx), stof(line.substr(idx + 1, line.size())));

			}
			catch (invalid_argument& e)
			{
				dout(string("Could not parse the packet error rate file. Error: ") + e.what(), true);
			}
		}
	}

	/* process symbol error rate lut */
	void read_symbol_error_rate_lut()
	{
		vector<string> file_contents, csvline;

		// Read the input file for simulation parameters
		getstream(input_dir + error_rate_file, file_contents);

		string encoding_scheme, x, y;
		for (auto& line : file_contents)
		{
			stringstream ss(line);
			getline(ss, encoding_scheme, ',');

			try
			{
				while (getline(ss, x, ',') && getline(ss, y, ','))
				{
					Global::SER_TABLE[encoding_scheme].emplace(stof(x), stod(y));
				}
			}
			catch (invalid_argument& e)
			{
				dout(string("Could not parse the symbol error rate file. Error: ") + e.what(), true);
			}
		}
	}

	void json_parser(string input_file)
	{
		Json::Value root;
		ifstream ifs;
		ifs.open(input_file);

		Json::CharReaderBuilder builder;
		builder["collectComments"] = false;
		JSONCPP_STRING errs;
		if (!parseFromStream(builder, ifs, &root, &errs)) {
			cout << errs << endl;
			throw runtime_error("Parser problem: " + num2str(EXIT_FAILURE));
		}

		/* helper function to break the cvs delimeters */
		const auto get_tokens = [](string input) {

			//auto input = jinput.asString();
			size_t idx2 = input.find(',');
			vector<string> out;

			if (idx2 < string::npos)
			{
				size_t idx = 0;
				while (idx2 < string::npos)
				{
					out.emplace_back(input.substr(idx, idx2 - idx));
					idx = idx2 + 1;
					idx2 = input.find(',', idx2 + 1);
				}
				out.emplace_back(input.substr(idx, idx2 - idx));
			}
			return out.size() ? out : vector<string>{ 1, input };
		};

		/* gather the data */
		unordered_map<string, vector<string>> inputs;
		for (Json::Value::const_iterator outer = root.begin(); outer != root.end(); outer++)
		{
			if (outer.key().compare("stations") == 0
				|| outer.key().compare("duration-milliseconds") == 0
				|| outer.key().compare("backoff") == 0)
			{
				for (Json::Value::const_iterator inner = (*outer).begin(); inner != (*outer).end(); inner++)
				{
					inputs.emplace(inner.key().asString(), get_tokens((*inner).asString()));
				}
			}
			else
			{
				inputs.emplace(outer.key().asString(), get_tokens((*outer).asString()));
			}
		}

		try
		{
			Global::traffic_type = inputs["connection-type"][0] == "tcp" ? 1 : 0;

			int idx = 0;
			for (auto& name : inputs["names"])
				Global::sta_name_map.emplace(name, idx++);

			Global::sta_name_map.emplace(Global::ap_station, idx++);
			Global::station_count = Global::sta_name_map.size();

			for (auto& txpower : inputs["transmit-power-dbm"])
				Global::txpowers.emplace_back(stod(txpower));

			if (Global::txpowers.size() != Global::sta_name_map.size())
				error_out("Transmit powers list is not the same size as station list");

			Global::data_pack_size = stod(inputs["databytes-bytes"][0]);
			Global::produration = stod(inputs["program"][0]) * 1000;
			Global::simduration = stod(inputs["simulation"][0]) * 1000;
			Global::DEBUG_END = stod(inputs["endtime"][0]) * 1000;

			Global::data_fragments = stod(inputs["segments"][0]);
			Global::frequency = stod(inputs["frequency"][0]);
			Global::bandwidth = stod(inputs["bandwidth"][0]);
			Global::adapt_int_tout = inputs["adaptive-timeout"][0] == "yes" ? true : false;

			Global::prop_factor = stof(inputs["pfactor"][0]);
			Global::chwindow = stoi(inputs["channel-window"][0]);
			Global::aCWmax = stoi(inputs["aCWmax"][0]);
			Global::aCWmin = stoi(inputs["aCWmin"][0]);

			if (Global::aCWmin > Global::aCWmax)
				error_out("CWmin is greater than CWmax");

#ifdef REDUNDANT_RETRIES
			if (stoi(inputs["relimit"][0]) > 0)
				error_out("Retry number problem");
			Global::dot11ShortRetryLimit = stoi(inputs["relimit"][0]);
#else
			uint retry_limit = stoi(inputs["relimit"][0]);
			uint validretry = log2(Global::aCWmax) - log2(Global::aCWmin) + 1.0;
			Global::dot11ShortRetryLimit = validretry != retry_limit ? validretry : retry_limit;
#endif

			/* setup the traffic generator and random number of generator */

			for (auto& seed : inputs["seeds"])
			{
#ifndef DETERMINISTIC
				Global::seeds.emplace_back(std::random_device()());
#else
				Global::seeds.emplace_back(stoul(seed));
#endif
			}

			if (inputs["trafficload-megabits"].size() != (Global::sta_name_map.size() - 1) * 2)
				error_out("Traffic load list is not twice as long as the station list for downstream and upstream link");

			/* create the connections and set the medium load */
#ifndef AP_MODE
			if (inputs["connections"].size() != Global::sta_name_map.size())
				error_out("Connections list is not the same as station list");

			for (int downstream_id = 0; downstream_id < inputs["connections"].size(); ++downstream_id)
			{

				auto& connection = inputs["connections"][downstream_id];
				uint source = stoi(connection.substr(0, connection.find(":")));
				uint dest = stoi(connection.substr(connection.find(":") + 1));
				Global::connections.create(source, dest);

				auto& loads      = inputs["trafficload-megabits"];
				auto upstream_id = (loads.size() / 2) + downstream_id;

				Global::connections.setload(source, dest, stod(loads[downstream_id]));
				Global::connections.setload(dest, source, stod(loads[upstream_id]));
#else
			for (int downstream_id = 0; downstream_id < Global::sta_name_map.size() - 1; ++downstream_id)
			{
				Global::connections.create(downstream_id, Global::sta_name_map[Global::ap_station]);

				auto& loads      = inputs["trafficload-megabits"];
				auto  ap_id      = Global::station_count - 1;
				auto upstream_id = (loads.size() / 2) + downstream_id;

				Global::connections.setload(downstream_id, ap_id, stod(loads[downstream_id]));
				Global::connections.setload(ap_id, downstream_id, stod(loads[upstream_id]));
#endif
			}
		}
		catch (invalid_argument& arg)
		{
			dout("Arg: " + string(arg.what()) + ". Was not a valid argument to convert from the json input file", true);
		}
		catch (out_of_range& arg)
		{
			dout("Arg: " + string(arg.what()) + ". Was not a valid index to reference from, in the json input file", true);
		}
	}

	/* returns the input file as is if asked */
	void readfile(string filename, vector<vector<string>>& out)
	{
		vector<string> file_contents, csvline;
		getstream(filename, file_contents);
		out.clear();

		// Read the input file for simulation parameters

		for (auto& fline : file_contents)
		{
			if (fline[0] == '#' || fline[0] == '\n') continue;

			if (fline.back() == '\n') fline.pop_back(); // delete \n

			if (fline.back() == ',') fline.pop_back(); // delete ,

			int delims = count_if(fline.begin(), fline.end(), [](char c) {return c == ',';  });

			stringstream ss(fline);
			csvline.resize(delims + 1);

			for (int i = 0; i <= delims; ++i)
			{
				getline(ss, csvline[i], ',');
			}

			out.push_back(csvline);
			csvline.clear();
		}
	}

	void copy_input_to_log(Logger*& log, const string& inputfile, string& output_path)
	{
		vector<string> file_contents;
		getstream(inputfile, file_contents);

		string token_conns = "connections\": \"";
		string token_seeds = "seeds\": \"";
		string token_retry = "relimit\": \"";
		string token_load = "trafficload-megabits\": \"";
		string load_data_from_input = "";

		/* 3 conditions where the inputs will be re-interpreted by the program */
		vector<string> output_buff;

		for (auto& line : file_contents)
		{
			auto idx = line.find(token_conns);
			if (idx != string::npos)
			{
				auto idx2 = line.rfind("\"");
				idx += token_conns.size();
				string connections = Global::connections.to_string();
				line.replace(idx, idx2 - idx, connections);
			}

			idx = line.find(token_seeds);
			if (idx != string::npos)
			{
				auto idx2 = line.rfind("\"");
				idx += token_seeds.size();
				string seeds = "";
				for (auto& seed : Global::seeds)
				{
					seeds += num2str(seed) + ",";
				}
				seeds.pop_back();
				line.replace(idx, idx2 - idx, seeds);
			}

			idx = line.find(token_retry);
			if (idx != string::npos)
			{
				auto idx2 = line.rfind("\"");
				idx += token_retry.size();
				auto retry_str = num2str(Global::dot11ShortRetryLimit);
				line.replace(idx, idx2 - idx, retry_str);
			}

			if (load_data_from_input.empty())
			{
				idx = line.find(token_load);
				if (idx != string::npos)
				{
					auto ite = line.find_first_of("0123456789", idx + token_load.size());
					auto ite2 = line.find("\"", ite);
					load_data_from_input  = line.substr(ite, ite2 - ite);
				}
			}

			output_buff.emplace_back(line);
		}

		string filename = "Summary_\[" + load_data_from_input + "\]";
		log = new Logger(output_path, filename);

		log->writeline("=================================== INPUT FILE =====================================");
		for (auto& line : output_buff)
			log->write(line);
		log->writeline("====================================================================================");
	}
}
