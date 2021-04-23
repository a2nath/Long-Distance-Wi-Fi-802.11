
Copyright (C) 2017 [Abhimanyu Nath](mailto:a2nath@uwaterloo.ca?subject=[GitHub]%20Long%20Distance%20Wifi%20Simulator)


# README #

Introduction: 

This repository holds source-code for my long-distance Wi-Fi network simulation model originally used to defend my thesis at Montana Tech of the University of Montana, November 2017. I will be expanding the functionality later as time permits.

If you’re interested in rapid prototyping and need to get up to speed with data structures of my network simulator, all explanation is hosted in here digitalcommons.mtech.edu/grad_rsch/141. You can also read the results of the simulations and take note of what exactly I changed and omitted from the original specification as stated in the IEE802.11-2012 standard (preview here: evs.ee/preview/iso-iec-ieee-8802-11-2012-en.pdf).


Goal of the simulations:

It is to show that within 50-100 km or more it is feasible to use 802.11a as a replacement for cellular networks in mountainous regions such as western Montana. This project is another WiLD network-related research where a set of MAC-layer subroutines are modified, to allow stations following CSMA/CA protocols to coordinate with other stations correctly after long signal propagation delays. WiLD stands for Wi-Fi Long Distance. And note that this project purely focuses on the feasibility of connecting distant clients separated by 1-hop, thus it does not explore the possibility of hybrid analysis using multiple hops (yet).

The following images depict non-line-of-sight issues between stations as returned from the SPLAT RF analysis:
<p align="center">
<img src="images/sbjr-nm.png?raw=true" width="500" alt="Non-line of sight terrain slice"/>
<img src="images/Picture1.png?raw=true" width="500" alt="Non-line of sight terrain slice"/>
<img src="images/sb-sm.png?raw=true" width="500" alt="Non-line of sight terrain slice"/>
</p>

Efforts to improve error-recovery: 

1. Convolutional codes: are used to improve the chances of recovering errors in the packets given certain interference-noise conditions using Vertibi algorithm. Significant improvement can be seen in the PER vs. SINR curves when convolutional codes are used FEC. 

<p align="center">
<img src="images/PER_convo_encode.png?raw=true" width="600" alt="Convolutional Encoded Graph">
</p>

2. Changes in the IEEE 802.11 MAC Layer: first of which is the flexibility of aCWMin and aCWMax parameters for the exponential backoff algorithm which is the smallest and largest possible windows used to draw a random number from. This allows the Wi-Fi clients for specific test scenarios to wait for a higher average amount of time for the response frame (PPDU) to reach the sender of data; thereby boosting throughput and reducing chances of re-transmissions. Second, optimization is performed on the AP "firmware" (station) which gives the device adaptive mode of operation. This mode lets the access-point to extend the length of the CTS/ACK timeout intervals as per round trip propagation time of the signal from a given client. Since the AP has the highest amount of traffic loading, it makes sense to perform this optimization to boost throughput and make that trade-off with slightly higher latencies on the client-side. This does not affect the length of NAV which only relies on the transmission duration of the CSMA frame exchange and interframe-spacing. RTS/CTS NAV have been made interruptible to allow the exchange of frames when NAV is initiated at non-legitimate moments of the frame exchange (stemming mainly from temporary hidden-mode properties) as displayed in this long-distance propagation wireless model. An additional DATA-timeout interval timer has been added as a fail-safe to make sure that stations do not have periods of inactivity resulting from lack of DATA frame from the sender.

<p align="center">
<img src="images/figure4.png?raw=true" width="800" alt="Simulator Inputs and Outputs">
</p>

How to setup:

There are two modes of simulations, one of them is the log-only simulations that ignores GUI libraries and does not use the QT package. It is possible that the compiler will complain about QT dependencies, and if it does, just create a new empty C++ project on Visual Studio and copy the source-code. The other mode of operation requires you do download 8-9 GB of QT packages before you can get going. You don't need to change anything in the source code as long as you setup the QT package correctly for Visual Studio. You don't need to re-configure dependencies or paths as far as I recall. Follow this tutorial to setup QT: youtu.be/P6Mg8FpFPS8

Where to look for results:

The GUI component of the project allows you to visualize the timeline for various stations and across several milliseconds. You are advised to keep the visualization duration short, i.e. up to 120,000 µs due to limitations with the GUI package. Put a breakpoint before the execution of GUI and change the starting simulation time to show a timeline from the middle of the simulation. You're welcome to use a different GUI package, I didn't have time to work on that aspect. To enable the display of the timeline feature, enable the flag in the common.h file ("SHOWGUI")

To check out the simulation statistics, go to the folder of the source code, and look for the Results folder. Your results should be time-stamped. If you're running the long-distance model, then the folden names will be appended with BO parameters.


How to contribute: 

If you want to contribute to the project, contact me and we'll setup a time for our discussion/code-review. This project was solely made by me. Some of the error conditions must never take place as a result of extensive debugging. If you encounter backoff errors and errors from various timers, then you have seriously broken one or more parts of the code - just a hint. In some cases of the debug, it is virtually impossible to debug the code without the timeline. Avoid conditional breakpoints which will slow down the simulations by several orders of magnitude, especially when you're waiting to reach a certain simulation time. The original software used to create this model was Visual Studio 2015, and QT 5.8.
