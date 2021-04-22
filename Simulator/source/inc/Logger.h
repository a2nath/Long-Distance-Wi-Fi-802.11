#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <ctime>
using namespace std;

class Logger
{
private:
	string name;
	ofstream outputfile;
public:
	Logger() {}
	~Logger() { done(); }
	Logger(string filenamepath) : name(filenamepath + ".txt")
	{
		outputfile.open(name);
	}

	void printstatus(vector<string> &filecontents, string seeds, string retrylim, int idx)
	{
		writeline("============================================================");
		for (int i = 0; i < filecontents.size()-1; ++i)
		{
			writeline(i == idx ? seeds.substr(0, seeds.length() - 1) : filecontents[i]);
		}
		writeline("relimit," + retrylim);
		writeline("============================================================");
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
		try {
			if (!outputfile)
				throw "get fucked";
		}
		catch (string s)
		{

		}

		outputfile << d;
	}

	std::string getname()
	{
		return name;
	}
	void done() { outputfile.close(); }
};