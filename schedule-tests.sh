#!/usr/bin/bash

# Use timeout to have tcpdump capture for a given number of seconds
# You'll have to change the path the file is saved to to match your vm setup.
# fast is a github project (https://github.com/sindresorhus/fast-cli) using nodejs to
# query fast.com from the command line
sudo timeout 45 /usr/bin/tcpdump -i enp0s8 -w "/home/client/fastpcap-$(date "+%H").pcap" &
/usr/local/bin/fast
