#!/bin/sh

CURRPWD=$(pwd) # current dir
OUT=$CURRPWD/model.out
LOG=$CURRPWD/last_compile.log

rm $OUT
cd ../source/src
files="Common.cpp Frames.cpp GUI.cpp Program.cpp Station_main.cpp Station_timers.cpp Timers.cpp TrafficGen.cpp WiFi_Main.cpp"
#options="-ftime-report"
#options="-pass-exit-codes"
macro=""
macro=$macro" -D DETERMINISTIC"
#macro=$macro" -D SHOWGUI"
macro=$macro" -D SHOWOUT"
#macro=$macro" -D REDUNDANT_RETRIES"
#macro=$macro" -D GUISTART=0"
#macro=$macro" -D GUIEND=1000"

echo -n "Compiling "
start=`date +%s`

if [ "$1" = "-release" ]; then
	echo "release build...\n"
	g++ $files -o $OUT -std=c++11 $options $macro -Wall 2>&1 | tee $LOG
else
	echo "debug build...\n"
	g++ $files -o $OUT -std=c++11 $options $macro -Wall -g 2>&1 | tee $LOG
fi

end=`date +%s`

# g++ exit code instead of using g++ flag
gcc_exit_code=$(grep ": error:" $LOG)
if [ -z "$gcc_exit_code" ]; then
	exit_code=0
else
	exit_code=1
fi

# give confirmation of compilation
if [ -f $OUT ]; then
	echo "\n\n======================================================================================="
	echo "program compiled on $(date) and took $(( $end - $start )) seconds with exit code $exit_code"
	echo "======================================================================================="
fi
