#!/usr/bin/bash

sudo timeout 60 /usr/bin/tcpdump -i enp0s8 -w "/home/client/results/fastcom/fastpcap-thr-$(date "+%H").pcap" &
/usr/local/bin/fast --upload --json >> "/home/client/results/fastcom/results.json"
