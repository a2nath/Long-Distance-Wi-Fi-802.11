#pragma once
#include <vector>
#include "Frames.h"
#include "common.h"
#include <iostream>
using namespace std;

/*
*
Keeps track of TX and RX frames in a given delta event for each station.
Input: delta time (double), number of stations (int)
*/
class Event
{
private:
	double timestamp;
	vector<Frame> frameTypeStationTX, frameTypeStationRX;
public:
	Event(double etime, int stationCount) :
		frameTypeStationRX(stationCount), frameTypeStationTX(stationCount), timestamp(etime)
	{
		cout << "RX frame vector size is: " << frameTypeStationRX.size() << endl;
		cout << "TX frame vector size is: " << frameTypeStationTX.size() << endl;
	}

	void setStationTX(int stID, Frame frame)
	{
		frameTypeStationTX[stID] = frame;
	}

	void setStationRX(int stID, Frame frame)
	{
		frameTypeStationRX[stID] = frame;
	}

	Frame getStationTX(int stID)
	{
		return frameTypeStationTX[stID];
	}

	Frame getStationRX(int stID)
	{
		return frameTypeStationRX[stID];
	}

	double getTime()
	{
		return timestamp;
	}
};