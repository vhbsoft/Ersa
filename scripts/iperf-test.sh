#!/usr/bin/bash

sudo timeout 60 /usr/bin/tcpdump -i enp0s8 -w "/home/client/results/iperf/iperfpcap-thr-$(date "+%H").pcap" &
/usr/bin/iperf3 -c iperf3.moji.fr -f m >> "/home/client/results/iperf/results"
