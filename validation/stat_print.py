import subprocess as sb
import os
import sys


os.chdir(str(sys.argv[1]))
def grepthis(command,shell):
	hosts_process = sb.Popen(command, shell=True, stdout= sb.PIPE)
	output, hosts_err = hosts_process.communicate()
	output = output.split('\n')[0:-1]
	if shell == True:
		output.kill()
	return output

retry_files =[f for f in os.listdir('.') if f.startswith("Station_")]
station0 = [i for i in retry_files if i.startswith("Station_0")][0]
station1 = [i for i in retry_files if i.startswith("Station_1")][0]
station2 = [i for i in retry_files if i.startswith("Station_2")][0]
station3 = [i for i in retry_files if i.startswith("Station_3")][0]

command = 'grep "del" "' + station0 + '" | tail -1'
hosts_process = sb.Popen(command, shell=True, stdout= sb.PIPE)
sta0, hosts_err = hosts_process.communicate()

command = 'grep "del" "' + station1 + '" | tail -1'
hosts_process = sb.Popen(command, shell=True, stdout= sb.PIPE)
sta1, hosts_err = hosts_process.communicate()

command = 'grep "del" "' + station2 + '" | tail -1'
hosts_process = sb.Popen(command, shell=True, stdout= sb.PIPE)
sta2, hosts_err = hosts_process.communicate()

command = 'grep "del" "' + station3 + '" | tail -1'
hosts_process = sb.Popen(command, shell=True, stdout= sb.PIPE)
sta3, hosts_err = hosts_process.communicate()

last_packet_del_us = [sta0, sta1, sta2, sta3]
last_packet_del_us = [float(i.split(' ')[0]) for i in last_packet_del_us]

#---- do the math ---
command = 'grep "Packets transmitted: " -r --include="Summary*"'
packets_transmitted_all = grepthis(command,False)
vector = [float(i.split()[-1])*1500*8 for i in packets_transmitted_all]
throughput = [s/last_packet_del_us[idx] for idx,s in enumerate(vector)]

command = 'grep "relimit" -r --include="Summary*"'
limit = grepthis(command,False)[0]
limit = int(limit.split(',')[-1])

command = 'grep "aCWmin" -r --include="Summary*"'
cwmin = grepthis(command,False)[0]
cwmin = int(cwmin.split(',')[-1])

command = 'grep "aCWmax" -r --include="Summary*"'
cwmax = grepthis(command,False)[0]
cwmax = int(cwmax.split(',')[-1])

print "\n\n" + str(cwmax) + "," + str(cwmin) + "," + str(limit)

command = 'grep "Average latency," -r --include="Summary*"'
hosts_process = sb.Popen(command, shell=True, stdout= sb.PIPE)
latency_out, hosts_err = hosts_process.communicate()
latency_out = latency_out.split('\n')[0:-1]
latency_out = [i.split(',')[-1].rstrip() for i in latency_out]

command = 'grep "Packets dropped: " -r --include="Summary*"'
hosts_process = sb.Popen(command, shell=True, stdout= sb.PIPE)
dropped_out, hosts_err = hosts_process.communicate()
dropped_out = dropped_out.split('\n')[0:-1]
dropped_out = [i.split()[-1] for i in dropped_out]

print "Dropped,Latency,Throughput"
for idx,s in enumerate(throughput):
	print ','.join([dropped_out[idx],latency_out[idx],str(s)])
print '\nDone'