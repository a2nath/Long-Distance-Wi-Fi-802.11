import os
import sys
import numpy as np
from copy import copy
import subprocess
from math import ceil
from time import sleep
none = -1

directory = 'analysis/'

os.chdir(str(sys.argv[1]))
print "dir:"+ str(sys.argv[1])
allfiles = [f for f in os.listdir('.') if os.path.isfile(f)]
stations = [f for f in os.listdir('.') if f.startswith("Station_")]
if "All_diffs.txt" in allfiles:
	os.remove("All_diffs.txt")
if "retry0.txt" in allfiles:
	os.remove("retry0.txt")
if "retry1.txt" in allfiles:
	os.remove("retry1.txt")
if "retry2.txt" in allfiles:
	os.remove("retry2.txt")
if "retry3.txt" in allfiles:
	os.remove("retry3.txt")
if "retry4.txt" in allfiles:
	os.remove("retry4.txt")
if "retry5.txt" in allfiles:
	os.remove("retry5.txt")
if "retry6.txt" in allfiles:
	os.remove("retry6.txt")
if "try7.txt" in allfiles:
	os.remove("try7.txt")

os.system("grep -nr --include='Station*.txt' 'difs start' >difs_start.txt")
for retry in range(7):
	os.system("grep -nr 'retry " + str(retry) + "' --include='Station*.txt' -A5 >  retry" + str(retry) + ".txt")
os.system('grep -nr --include="Station*.txt" "try 7" -B2 -A1 > try7.txt')

print "=======================\n"

def retry_validate(idx,output,token):
	found = False;
	idx2 = copy(idx) + 1
	while output[idx2].find("retry ") == none and output[idx2] != '':
		if output[idx2].find(token) > none:
			found = True
			break
		idx2 += 1
	return found;

def gettime(line,token):
	if token == ":":
		return int(line.split(token)[2].split(' ')[0])
	else:
		return int(line.split(' ')[1].split(token)[2])

def grepit(command,shell):
	if shell == True:
		hosts_process = subprocess.Popen(command, shell=True, stdout= subprocess.PIPE)
		hosts_out, hosts_err = hosts_process.communicate()
		sleep(1)
		hosts_process.kill()
	else:
		hosts_process = subprocess.Popen(command, stdout= subprocess.PIPE)
		hosts_out, hosts_err = hosts_process.communicate()
	return [hosts_out, hosts_err]

# difs test
with open("difs_start.txt",'r') as f:
	fout = f.read().splitlines()
sta = [int(i.split(' ')[0][-1:]) for i in fout]
starts = [int(i.split(':')[2].split(' ')[0]) for i in fout]
station_map = dict()
for idx in range(len(starts)):
	if sta[idx] not in station_map:
		station_map[sta[idx]] = []
	station_map[sta[idx]].append(starts[idx])

for i in station_map:
	d_station_file = open("diff_sta" + str(i) + ".txt","w+")
	print "Calculating DIFS difference for " + str(i)
	diffs = np.diff(station_map[i])
	sta_starts = station_map[i]
	for idx in range(len(diffs)):
		d_station_file.write(str(sta_starts[idx+1]) + " difs start:" + str(diffs[idx]) + "\n")
	d_station_file.close();

difs_file = open("All_diffs.txt","w+")
sorted_idx = [i[0] for i in sorted(enumerate(starts), key=lambda x:x[1])]
starts.sort()
sta = [sta[i] for i in sorted_idx]
diffs = np.diff(starts)

for idx in range(len(diffs)):
	difs_file.write(str(starts[idx+1]) + " difs start:" + str(diffs[idx]) + " [" + str(sta[idx+1]) + "]\n")
difs_file.close()

# backoff window tests
slotmap = {"6":"/31","5":"/63","4":"/127","3":"/255","2":"/511","1":"/1023"}
retry_files =[f for f in os.listdir('.') if f.startswith("retry") or f.startswith("try")]
bofile = open("backoff_results.txt","w+")

for file in retry_files:
	print "retry file: " + file
	with open(file,'r') as f:
		fileout = f.read().splitlines()
		fileout = ["--"] + fileout
		if file.endswith("0.txt"):
			retry0 = [i for i in fileout if i.find("retry 0") > none]
			entries = len(retry0)

			retry_idx = [i for i,s in enumerate(fileout) if s.find("retry 0") > none]
			retries_dropped = [fileout[i] for i in retry_idx if retry_validate(i,fileout + [''],"frame dropped") == True]
			dropped_lines = [i for i in fileout if i.find("frame dropped") > none]
			drop_count = len(dropped_lines)
			successes = len([f for f in fileout if f.find("del ") > none])

			if (entries - drop_count != successes):
				bofile.write("[retry0]:conduct additional tests, LOL\n")
			else:
				bofile.write("[retry0]:verified\n")

		elif file == "try7.txt":
			bofile.write("[ try" + file[:-4][-1:] + " ]:")
			try_idx = [i for i, j in enumerate(fileout) if j.find("try 7") > none]
			negated_lines = [i for i in fileout if i.find("frame negated") > none]
			tries = len(try_idx)
			difs_start = len([i for i, j in enumerate(fileout) if j.find("difs start until") > none])

			check_this = []
			for i in try_idx:
				try_time_ac = gettime(fileout[i],":")
				if "difs" not in fileout[i-1] and "difs" not in fileout[i-2]:
					print "--\nNo difs before the try 7:" + fileout[i]
					check_this.append(fileout[i])
				else:
					if "difs" in fileout[i-1]:
						difs_line = fileout[i-1]
					elif "difs" in fileout[i-2]:
						difs_line = fileout[i-2]
					time = int(difs_line.split(' ')[-1])
					try_time_th = ceil(time/float(9))*9
					if try_time_th != try_time_ac:
						print "--\nDIFS time wrong:" + fileout[i]
						check_this.append(fileout[i])

			negated_times = [gettime(i,"-") for i in negated_lines]
			try_times = [gettime(fileout[i],":") for i in try_idx]
			for time in negated_times:
				if time not in try_times:
					print "--\nRTS negation problem:" + fileout[try_idx]
					check_this.append(fileout[try_idx])

			# ----- check for overall problems -----------------------------
			if len(check_this) > 0:
				for strings in check_this:
					bofile.write(strings + "\n")
			else:
				bofile.write("verified\n")
		else:
			retry_num = file[:-4][-1:]
			bofile.write("[retry" + retry_num + "]:")
			invalid_lines = [i for i, s in enumerate(fileout) if s.startswith("retry")]
			indices = [i for i, j in enumerate(fileout) if j.find("retry " + retry_num) > none]

			if len(invalid_lines) > 0:
				indices = [i for i in indices if i not in range(invalid_lines[-1] + 1)]

			retries = len([f for f in fileout if f.find("retry " + retry_num) > none])
			slot_times = len([f for f in fileout if f.find(slotmap[retry_num]) > none])
			successes = len([f for f in fileout if f.find("del ") > none])
			check_this = []
			if slot_times + successes != retries:
				for idx in indices:
					if  retry_validate(idx,fileout + [''],"del") == False and retry_validate(idx,fileout + [''],slotmap[retry_num]) == False:
						station = fileout[idx].split(':')[0]
						line = fileout[idx][fileout[idx].index('qsize'):]
						command = "grep -n '" + line + "' '" + station + "' -A7"
						out = grepit(command,False)[0]
						if  retry_validate(0,out.split('\n'),"del") == False and retry_validate(0,out.split('\n'),slotmap[retry_num]) == False \
							and retry_validate(idx,fileout + [''],"DATA release") == False:
							print "--\n" + out
							check_this.append(idx)
				if len(check_this) > 0:
					for idx in check_this:
						bofile.write(fileout[idx] + "\n")
				else:
					bofile.write("verified\n")
			else:
				bofile.write("verified\n")

dropped_times = [gettime(i,'-') for i in dropped_lines]
retry_times = [gettime(i,':') for i in retries_dropped]
bofile.write("\n-----\n[Retry 0 Analysis]\nDropped-Time:Difference(Dropped Time - RETRY0 Release):\n")
for i in range(len(dropped_times)):
	bofile.write(str(dropped_times[i]) + ":" + str(dropped_times[i] - retry_times[i]) + (" <<<" if dropped_times[i] - retry_times[i] > 126 else "") + "\n")

recovered_frames = grepit("grep 'drop recovered' -r --include='Station*.txt'",False)[0].split('\n')[0:-1]
routput = grepit("grep 'Packets dropped' -r --include='Summary *.txt'",False)[0].split('\n')[0:-1]
dropped_reported = [int(s.split(': ')[-1]) for s in routput]

if sum(dropped_reported) == drop_count-len(recovered_frames):
	bofile.write("Recovered frames reported in Summary.txt file correctly\n")

command1 = "grep -r 'Slot count : ' --include='Station*.txt' | wc -l"
command2 = "grep -r ' Negating ' --include='Station*.txt' | wc -l"
command3 = "grep -r ', retry ' --include='Station*.txt' | wc -l"

slot_counts = grepit(command1,True)[0][0:-1]
slot_neg = grepit(command2,True)[0][0:-1]
retries = grepit(command3,True)[0][0:-1]

print "Slot count: " + slot_counts
print "Slots negated: " + slot_neg
print "Retries: " + retries
print "Net: " + str(int(slot_counts)-int(slot_neg)-int(retries))
bofile.write("\n-----\n[Retry > 0 Analysis]\n")
bofile.write("Net slot count: " + str(int(slot_counts)-int(slot_neg)-int(retries)) + "\n")
bofile.close()
