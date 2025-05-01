#!/usr/bin/bash

sudo timeout 60 /usr/bin/tcpdump -i enp0s8 -w "/home/client/results/speedcheck/speedcheckpcap-thr-$(date "+%H").pcap" &
/usr/bin/python3 /home/client/speedcheck-test.py >> "/home/client/results/speedcheck/results"
