# README #

## Description 

Source code for long-distance Wi-Fi network simulation model originally used to defend my thesis at Montana Tech of the University of Montana, November 2017. I will be expanding the functionality later as time permits. If you’re interested in rapid prototyping and need to get up to speed with data structures of my network simulator, all explanation is hosted at the [University of Montana page](https://digitalcommons.mtech.edu/grad_rsch/141/). You can also read the results of the simulations and take note of what exactly I changed and omitted from the original specification as stated in the IEE802.11-2012 standard (preview here: `evs.ee/preview/iso-iec-ieee-8802-11-2012-en.pdf`).

## Tested on 

* Visual Studio 2015
* MVS C++11 
* SRTM Models v3
* QT 5.8 drawing library

## Goal 

Simulations show that within 50-100 km or more it is feasible to use 802.11a as a replacement for cellular networks in mountainous regions such as western Montana. This project is another WiLD network-related research where a set of MAC-layer subroutines are modified, to allow stations following CSMA/CA protocols to coordinate with other stations correctly after long signal propagation delays. WiLD stands for Wi-Fi Long Distance. And note that this project purely focuses on the feasibility of connecting distant clients separated by 1-hop, thus it does not explore the possibility of hybrid analysis using multiple hops (yet).

The following images depict non-line-of-sight issues between stations as returned from the SPLAT RF analysis:
<p align="center">
<img src="images/sbjr-nm.png?raw=true" width="500" alt="Non-line of sight terrain slice"/>
<img src="images/Picture1.png?raw=true" width="500" alt="Non-line of sight terrain slice"/>
<img src="images/sb-sm.png?raw=true" width="500" alt="Non-line of sight terrain slice"/>
</p>

## Efforts to improve error-recovery

1. Convolutional codes: are used to improve the chances of recovering errors in the packets given certain interference-noise conditions using Vertibi algorithm. Significant improvement can be seen in the PER vs. SINR curves when convolutional codes are used FEC. 

<p align="center">
<img src="images/PER_convo_encode.png?raw=true" width="600" alt="Convolutional Encoded Graph">
</p>

2. Changes in the IEEE 802.11 MAC Layer: first of which is the flexibility of `aCWMin` and `aCWMax` BO parameters for the exponential backoff algorithm which is the smallest and largest possible windows used to draw a random number from. This allows the Wi-Fi clients for specific test scenarios to wait for a higher average amount of time for the response frame (PPDU) to reach the sender of data; thereby boosting throughput and reducing chances of re-transmissions. Second, optimization is performed on the AP "firmware" (station) which gives the device adaptive mode of operation. This mode lets the access-point to extend the length of the CTS/ACK timeout intervals as per round trip propagation time of the signal from a given client. This does not affect the length of NAV which only relies on the transmission duration of the CSMA frame exchange and interframe-spacing. RTS/CTS NAV have been made interruptible to allow the exchange of frames when NAV is initiated at non-legitimate moments of the frame exchange (stemming mainly from temporary hidden-mode properties) as displayed in this long-distance propagation wireless model. An additional DATA-timeout interval timer has been added as a fail-safe to make sure that stations do not have periods of inactivity resulting from lack of DATA frame from the sender.

<p align="center">
<img src="images/figure4.png?raw=true" width="800" alt="Simulator Inputs and Outputs">
</p>

## Setup

There are two modes of simulations, one of them is the log-only simulations that ignores GUI libraries or the QT package, and thus the timeline visualization. It is possible that the compiler will complain about QT dependencies, and if it does, disable the flag `#define SHOWGUI` in the file, `common.h`. 

The other mode of operation with the flag enabled requires you do download 8-9 GB of QT packages before you can get going. You don't need to change anything in the source code as long as you setup the QT package correctly for Visual Studio. Follow this tutorial to setup QT: `youtu.be/P6Mg8FpFPS8`

## Results

#### Visualization

The GUI component of the project allows you to visualize the timeline for various stations and across several milliseconds. You are advised to keep the visualization duration short, i.e. up to 120,000 µs due to limitations with the GUI package. Put a breakpoint before the execution of GUI and change the starting simulation time to show a timeline from the middle of the simulation. You're welcome to use a different GUI package, I didn't have time to work on that aspect. To enable the display of the timeline feature, enable the flag `#define SHOWGUI` in the file, `common.h`

#### Simulation Statistics

Look for the `Results` folder. Your results should be time-stamped. If you're running the long-distance model, then the folden names will be appended with `aCWMin` and `aCWMax` BO parameters.
