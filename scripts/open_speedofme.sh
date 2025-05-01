#!/usr/bin/bash

sudo timeout 60 /usr/bin/tcpdump -i enp0s8 -w "/home/client/results/speedofme/speedofmepcap-thr-$(date "+%H").pcap" &
timeout 60 /usr/bin/node /home/client/CS686-3-Project/speedofme/index.js & 
timeout 60 /usr/bin/firefox -headless "localhost:3000"
