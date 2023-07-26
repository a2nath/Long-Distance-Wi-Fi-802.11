import os
import sys
from shutil import copy2
from subprocess import PIPE, STDOUT, Popen

# station configuration parameters in Watts
station_transmit_power = 20
station_transmit_freq = 530

inputs = str(sys.argv).split()[1:]
inputs = [s[1:-2] for s in inputs]
transmitter =  inputs[0]
bool_hd = ""
receiver_station = ""
rx_ant_height = ""
topo = ""
new = " -olditm "
if len(inputs) > 4:
	bool_hd = '-hd'
	print "HD MODE ACTIVATED!"
	topo = 'topo'
	receiver_station = inputs[1]
	rx_ant_height = inputs[2]
elif len(inputs) > 3:
	if 'hd' in inputs and 'topo' in inputs and 'new' in inputs:
		bool_hd = '-hd'
		print "HD MODE ACTIVATED!"
		topo = 'topo'
		new = " "
	else:
		receiver_station = inputs[1]
		if 'hd' in inputs and 'topo' in inputs:
			bool_hd = '-hd'
			print "HD MODE ACTIVATED!"
			topo = 'topo'
		elif 'hd' in inputs and 'new' in inputs:
			bool_hd = '-hd'
			print "HD MODE ACTIVATED!"
			new = " "
		elif 'topo' in inputs and 'new' in inputs:
			topo = 'topo'
			new = " "
		else:
			if 'hd' in inputs:
				bool_hd = '-hd'
				print "HD MODE ACTIVATED!"
			elif 'topo' in inputs:
				topo = 'topo'
			elif 'new' in inputs:
				new = " "
			rx_ant_height = inputs[2]

elif len(inputs) > 2:
	if 'hd' in inputs and 'topo' in inputs:
		bool_hd = '-hd'
		print "HD MODE ACTIVATED!"
		topo = 'topo'
	elif 'hd' in inputs and 'new' in inputs:
		bool_hd = '-hd'
		print "HD MODE ACTIVATED!"
		new = " "
	elif 'topo' in inputs and 'new' in inputs:
		topo = 'topo'
		new = " "
	else:
		receiver_station = inputs[1]
		if 'hd' in inputs:
			bool_hd = '-hd'
			print "HD MODE ACTIVATED!"
		elif 'topo' in inputs:
			topo = 'topo'
		elif 'new' in inputs:
			new = " "
		else:
			rx_ant_height = inputs[2]
elif len(inputs) > 1:
	if 'hd' in inputs:
		bool_hd = '-hd'
		print "HD MODE ACTIVATED!"
	elif 'topo' in inputs:
		topo = 'topo'
	elif 'new' in inputs:
		new = " "
	else:
		receiver_station = inputs[1]



google_earth_file = [f for f in os.listdir(os.getcwd()) if f.endswith('.kml')][0]
with open(google_earth_file,'r') as f:
	lines = f.read().splitlines()

# -------------------- parsing the Google Earth file --------------------
idx = [i for i, s in enumerate(lines) if '<name>Butte</name>' in s]
lines = lines[idx[0]+3:]
namesi = [i for i, s in enumerate(lines) if '<name>' in s]
location = [s for i, s in enumerate(lines) if '<coordinates>' in s]
names = [lines[i].split('>')[1].split('<')[0].replace(' ','_').replace('-','_') for i in namesi]
idx = [i for i, s in enumerate(names) if s.find('(') != -1]
for i in idx:
    names[i] = names[i].split('(')[1][:-1]
longs = [s.split('>')[1].split('<')[0].split(',')[0][1:] for s in location]
lats = [s.split('>')[1].split('<')[0].split(',')[1] for s in location]
longitudes = {}
latitudes = {}

for idx, name in enumerate(names):
	longitudes[name] = longs[idx]
	latitudes[name] = lats[idx]

antheight_file = 'antenna_heights.txt'
with open(antheight_file,'r') as f:
	heights = f.read().splitlines()
heights = [s.split(' ') for s in heights]
antenna_heights = {}
for h in heights:
	antenna_heights[h[0]] = h[1]


print "\n------- Using " + transmitter + " as TX station -------"

xloc_tx = 0
yloc_tx = 0

if receiver_station != "":
	names = []
	names.append(transmitter)
	names.append(receiver_station)
	height_tx = antenna_heights[transmitter]
	height_rx = antenna_heights[receiver_station]
	antenna_heights = {}
	antenna_heights[transmitter] = height_tx
	print "height: " + rx_ant_height
	antenna_heights[receiver_station] = rx_ant_height if rx_ant_height != "" else height_rx

for name in names:
	# build the LRP file
	if name == transmitter:
		rec_file = open(name + '.lrp', "w")
	        rec_file.write("15.000"+"\n")
	        rec_file.write("0.005"+"\n")
	        rec_file.write("301.00"+"\n")
	        rec_file.write(str(station_transmit_freq)+"\n")
	        rec_file.write("5"+"\n")
	        rec_file.write("1"+"\n")
		rec_file.write(".5"+"\n")
		rec_file.write(".5"+"\n")
		rec_file.write(str(station_transmit_power))
		rec_file.close()
		xloc_tx = longitudes[name]
		yloc_tx = latitudes[name]
		print "TX station long:" +  str(-1 * float(xloc_tx))
		print "TX station lat:" + yloc_tx
		print transmitter + " antenna height (meters): " + antenna_heights[transmitter]

	# build the QTH file
	rec_file = open(name + '.qth', "w")
	rec_file.write(name + '\n')
	rec_file.write(latitudes[name] + '\n')
	rec_file.write(longitudes[name] + '\n')
	rec_file.write(antenna_heights[name] + ' m\n')
	rec_file.close()

print ""

# -------------------- define the Results folder before starting the simulations --------------------
directory = os.getcwd() + '/' + transmitter + '_results' + bool_hd + ("-new" if new == " " else "") + '/'
if not os.path.exists(directory):
	os.makedirs(directory)
if not os.path.exists(directory + 'qfiles/'):
	os.makedirs(directory + 'qfiles/')
if not os.path.exists(directory + 'graphs/'):
	os.makedirs(directory + 'graphs/')
if not os.path.exists(directory + 'topo/') and topo != "":
	os.makedirs(directory + 'topo/')
if topo != "":
	topo = " -o " + directory + "topo/"
# -------------------- perform the simulations and move the results --------------------
rec_file = open(directory + transmitter + '_summary.txt', "w")
for rx_sta in names:
	if rx_sta == transmitter:
		continue
	command = "splat" + bool_hd + " -t " + transmitter + ".qth -r " + rx_sta + ".qth -metric" + new + "-p " + directory + 'graphs/' + rx_sta + ".png -d " + os.getcwd() + "/sdf" + bool_hd + "/version-3" + ("" if topo == "" else topo + rx_sta)
	process = Popen(command, shell=True, stdout=PIPE, stderr=STDOUT)
	output, error = process.communicate()


	output_file = transmitter+"-to-"+rx_sta+".txt"
	if output.count('Path Loss Report written to: "' + output_file + '"') == 0 or output.count('Terrain plot written to: "' + directory + 'graphs/' + rx_sta +'.png"') == 0 or output.count('assumed as sea-level') > 0:
		raise Exception(rx_sta + ': did not execute this one')
		rec_file.close()
	else:
		with open(output_file,'r') as f:
			fileout = f.read().splitlines()
		heights = [s.split() for i, s in enumerate(fileout) if 'Ground elevation' in s]
		xdiff = float(longitudes[name]) - float(xloc_tx)
		ydiff = float(latitudes[name]) - float(yloc_tx)
		zdiff = str(abs(float(heights[0][2]) - float(heights[1][2])));
		distance = [s.split(' ')[3] for i, s in enumerate(fileout) if 'Distance to' in s][0]
		FSPL = [s.split(' ')[4] for i, s in enumerate(fileout) if 'Free space path' in s][0]
		LRPL = [s.split()[-2] for i, s in enumerate(fileout) if ('ITWOM Version 3.0 path' if new == " " else "Longley-Rice path") in s][0]
		SPL = [s.split(' ')[5] for i, s in enumerate(fileout) if 'Signal power level' in s][0]
		ernum = [s.split(' ')[3 if new == " " else 3] for i, s in enumerate(fileout) if ('ITWOM error number' if new == " " else 'Longley-Rice model') in s][0]
		rec_file.write(rx_sta + ":zdiff " + str(zdiff) + " m, distance " + distance + " km, FSPL " + FSPL + " dB, LRPL " + LRPL + " dB, SPL " + SPL + " dBm, error " + ernum + "\n")
		print rx_sta + " ... done" + " [" + LRPL + " dB]" + "[" + antenna_heights[rx_sta] + "]" + ("[" + [s.split('Mode of propagation: ')[1] for i, s in enumerate(fileout) if 'Mode of propagation' in s][0] + "]" if new == " " else "")
		os.rename(os.getcwd()+'/'+output_file, directory + output_file)
rec_file.write(str(station_transmit_power)+'\n')
rec_file.write(str(station_transmit_freq))
rec_file.close()
# -------------------- moving rest of the files into the output directory --------------------
os.rename(os.getcwd()+'/' + transmitter + '-site_report.txt', directory + transmitter + '-site_report.txt')
os.rename(os.getcwd()+'/' + transmitter + '.lrp', directory + transmitter + '.lrp')
copy2(os.getcwd()+'/'+antheight_file, directory)
qth_files = [f for f in os.listdir(os.getcwd()) if f.endswith('.qth')]
for qfile in qth_files:
	os.rename(os.getcwd()+'/'+qfile, directory + 'qfiles/' + qfile)

print "-------------------------------------------------\n"
print "Scipt finished running\n"
