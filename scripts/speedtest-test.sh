#!/usr/bin/bash

sudo timeout 60 /usr/bin/tcpdump -i enp0s8 -w "/home/client/results/speedtest/speedtestpcap-thr-$(date "+%H").pcap" &
/usr/bin/speedtest --secure >> "/home/client/results/speedtest/results.json"
