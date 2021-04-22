import os
import sys
#os.system('python master.py myplaces.kml 15')

hd = ""
txstation = str(sys.argv).split()[1:][0][1:-2]
if (len(str(sys.argv).split()) > 2):
	hd = "-" + str(sys.argv).split()[1:][1][1:-2]

txpower = 0
frequency = 0
print '\n--------------------- Using ' + txstation + ' as TX station ---------------'
stations = [f[:-(8 if hd == "" else 11)] for f in os.listdir(os.getcwd()) if f.endswith('_results' + hd)] #get the list of stations

# -------------------- shortlist stations based on the picked transmitter --------------------
viable_list = stations
if len(viable_list) == 0:
	print "Station results not populated from master script!"
print '\nGeographic coodinates found: ' + str(len(stations)) + '\n'

for name in stations:
	directory = os.getcwd() + '/' + name + '_results' + hd + '/'
	with open(directory + name + '_summary.txt','r') as f:
		lines = f.read().splitlines()

	#go through each line in a file and look at individual numbers
	for i,sta in enumerate(lines):
		sta_name = sta.split()[0].split(':')[0]
		if name == txstation and i == len(lines)-2:
			txpower = sta_name #really this is a number (power in Watts)
		elif name == txstation and i == len(lines)-1:
			frequency = sta_name #just a number (frequency in Mega sHertz)
		elif name == txstation and float(sta.split()[10]) > 148.0: #pathloss is too damn high so ignore this one
			if sta_name in viable_list:
				print "Removing station: " + sta_name
				viable_list.remove(sta_name)
			else:
				print "Warning: " + sta_name + " station results not present"



# -------------------- Now build the table --------------------
namtable = []
distable = []
pattable = []
print '\n---------------------------------------------------------------'
print '\nNow building tables. Geographic coodinates count: ' + str(len(viable_list)) + '\n'

for name in viable_list:

	directory = os.getcwd() + '/' + name + '_results' + hd + '/'
	with open(directory + name + '_summary.txt','r') as f:
		lines = f.read().splitlines()

	namtable_row = []
	distable_row = []
	pattable_row = []
	namtable_row.append(name + ',')
	distable_row.append(name + ',')
	pattable_row.append(name + ',')
	for sta in lines:
		sta_name = sta.split()[0].split(':')[0]
		if sta_name in viable_list:
			namtable_row.append(sta_name + ',')
			distable_row.append(sta.split()[4] + ',')
			pattable_row.append(sta.split()[10] + ',')
	namtable.append(namtable_row)
	distable.append(distable_row)
	pattable.append(pattable_row)
	print name + " ... built"

namtable[-1] = namtable[-1][:-1]
distable[-1] = distable[-1][:-1]
pattable[-1] = pattable[-1][:-1]

# -------------------- Write table to file --------------------
station_names = open('station_names.txt', "w")
distance_file = open('distance_table.txt', "w")
pathloss_file = open('pathloss_table.txt', "w")

for idx, row in enumerate(distable):
	for idy, col in enumerate(distable[idx]):
		station_names.write(namtable[idx][idy])
		distance_file.write(distable[idx][idy])
		pathloss_file.write(pattable[idx][idy])
	station_names.write('\n')
	distance_file.write('\n')
	pathloss_file.write('\n')
station_names.write(str(txpower)+'\n')
station_names.write(str(frequency))

station_names.close()
distance_file.close()
pathloss_file.close()



