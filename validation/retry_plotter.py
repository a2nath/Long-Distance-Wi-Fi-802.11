import os
import sys
import numpy as np

os.chdir(str(sys.argv[1]))
print "dir:"+ str(sys.argv[1])
station = -1

zero_only = 1 if str(sys.argv[2]) == "0" else 0
dropped_only = 1 if str(sys.argv[2]) == "dropped" else 0
if len(sys.argv) > 3:
	station = sys.argv[3]
station_to_find = 'Station*.txt' if station < 0 else 'Station_' + station + '*.txt'
if zero_only == 1:
	os.system("grep ', retry 0' -r --include='" + station_to_find + "' > rts_release.txt")
elif dropped_only == 1:
	os.system("grep ', frame dropped' -r --include='" + station_to_find + "' > rts_release.txt")
else:
	os.system("grep 'RTS release' -r --include='" + station_to_find + "' > rts_release.txt")
with open('rts_release.txt','r') as f:
	fout = f.read().splitlines()

retry_output = open("retry_output.txt","w+")

f = [int(i.split('qsize')[0].split(':')[-1]) for i in fout]
idx = [i[0] for i in sorted(enumerate(f), key=lambda x:x[1])]
sorted_lines = [fout[i] for i in idx]
f = [i.split('qsize')[0].split(':')[-1] for i in sorted_lines]
g = [sorted_lines[i][sorted_lines[i].index(f[i]):] for i,s in enumerate(f)]

ints = [int(i) for i in f]
diffs = np.diff(ints)

retry_output.write(g[0] + ",-" + ',1\n')
for i,s in enumerate(diffs):
	retry_output.write(g[i+1] + "," + str(s) + ',1\n')
retry_output.close()

print "DONE"
