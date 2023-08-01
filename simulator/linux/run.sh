#!/bin/sh

CURRPWD=$(pwd) # current dir
OUT=$CURRPWD/model.out
LOG=$CURRPWD/last_compile.log

cd ../source/src

echo -n "Compiling "
if [ "$1" = "-release" ]; then
	echo "release build...\n"
	g++ Common.cpp Frames.cpp GUI.cpp Program.cpp Station_main.cpp Station_timers.cpp Timers.cpp TrafficGen.cpp WiFi_Main.cpp -o $OUT -std=c++11 -Wall | tee $LOG
else
	echo "debug build...\n"
	g++ Common.cpp Frames.cpp GUI.cpp Program.cpp Station_main.cpp Station_timers.cpp Timers.cpp TrafficGen.cpp WiFi_Main.cpp -o $OUT -std=c++11 -Wall -g | tee $LOG
fi