#!/bin/sh


cd ../source/src

echo -n "Compiling "
if [ "$1" = "-release" ]; then
	echo "release build...\n"
	g++ Common.cpp Frames.cpp GUI.cpp Program.cpp Station_main.cpp Station_timers.cpp Timers.cpp TrafficGen.cpp WiFi_Main.cpp -std=c++11 -Wall | tee last_run.log
else
	echo "debug build...\n"
	g++ Common.cpp Frames.cpp GUI.cpp Program.cpp Station_main.cpp Station_timers.cpp Timers.cpp TrafficGen.cpp WiFi_Main.cpp -std=c++11 -Wall -g | tee last_run.log
fi
