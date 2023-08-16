#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir _mkdir
#define chdir _chdir
#else
#define mkdir(path) mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
#endif;

inline int create_dir(std::string);

using namespace std;

class Logger
{
private:
	string path;
	string main_file;
	string station_dir;
	vector<ofstream> writers; // file-writers for various profiling info written to diff files
	int writer_idx;

	int isDir(string& dir)
	{
		struct stat info;

		if (stat(dir.c_str(), &info) != 0)
			return 0;
		else if (info.st_mode & S_IFDIR)
			return 1;
		else
			return -1;
	}
public:
	Logger(string& _path, string _name) : path(_path), main_file(path + _name + ".txt")
	{
		writers.emplace_back(ofstream(main_file));
		writer_idx = 0;
	}

	Logger(int station_id, string& outputdir, string& station_name)
	{
		/* creat a new directory for station-specific data */
		station_dir = outputdir + "station " + to_string(station_id) + "/";
		create_dir(station_dir);

		string name = "station_" + to_string(station_id);
		path = station_dir + name + " [" + station_name + "]";
		main_file = path + "_verbose.txt";

		writers.emplace_back(main_file);
		writer_idx = 0; // main writer

	}

	~Logger() { done(); }

	void change_file(uint station = 0, string state = "")
	{
		if (state.empty())
		{
			writer_idx = 0;
			return;
		}

		if (state.compare(":: Packet Latency Profiling Info ::") == 0)
		{
			writers.emplace_back(ofstream(path + "_packet_latency_to_station_" + to_string(station) + ".txt"));
			++writer_idx;
		}
		else if (state.compare(":: Queue Size Profiling Info ::") == 0)
		{
			writers.emplace_back(ofstream(path + "_queue_size_to_station_" + to_string(station) + ".txt"));
			++writer_idx;
		}
	}

	void print_input_file(vector<string> &filecontents, string seeds, string retrylim, int idx)
	{
		writeline("=================================== INPUT FILE =====================================");
		for (int i = 0; i < filecontents.size()-1; ++i)
		{
			writeline(i == idx ? seeds : filecontents[i]);
		}
		writeline("relimit," + retrylim);
		writeline("====================================================================================");
	}
	template<class T>
	void writeline(T d)
	{
		write(d);
		write("\n");
	}
	template<class T>
	void writefor(vector<T> &doublevec, uint columns = 1)
	{
		++columns;
		uint index = 1;
		int i = 1;
		while(index <= doublevec.size())
		{
			if (i % columns > 0)
			{
				write(doublevec[index - 1]);
				write(",");
				++index;
			}
			else
				write("\n");
			++i;
		}
		write("\n");
	}
	template<class T>
	void write(T d)
	{
		auto& writer = writers[writer_idx];

		if (!writer)
			throw runtime_error("File IO error");

		writer << d;
	}

	std::string getname()
	{
		return main_file;
	}
	void done()
	{
		for (auto& writer : writers) writer.close();
	}
};
