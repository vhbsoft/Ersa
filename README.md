# CS686-3-Project

This repository contains code used for the project Evaluating the Effectiveness of Bandwidth Measurement Tools in the Presence of a Deceptive ISP during Spring 2024 by James Ambat, Jayanarayanan Jayaganesh, Ashutosh Malla, Lawrence Ng, and Maxwell Weidmer.

The original README containing information on how to configure the click router and run it with the modified classifier that enabled us to separate network packets between Bandwidth Measurement Tool traffic or not can be found in the section [CS686 Original Repository README below](#cs686-original-repository-readme-below).

The original repository contains only the following files/directories:
- click/
- evaluation/
- pcap_analysis_tool/
- scripts/
- speedofme/
- classifier.cc
- classifier.hh
- sandbox_tester.click

Additional work has been work to create 

## CS686 Original Repository README below

Move the `classifier.cc` and `classifier.hh` into the following directory:
```
/click/elements/standard
```

Compile the project:
```
cd /click
make clean
./configure --enable-all-elements --enable-local --enable-userlevel --disable-bsdmodule --disable-linuxmodule
make
```

Run the router: 
```
/click/bin/click /path_to_config/project.click
```

---

### Scheduling PCAPS using Cron Jobs

See the /scripts folder for an example script to start a packet capture using tcpdump, and then run the measurement tool.
I use $(date "+%H") in the filename of the pcap, so that tcpdump doesn't overwrite the file every time.


You must include the .pcap at the end of the tcpdump file name, otherwise you'll run into permissions issues
when using the packet analysis tool created by James, specifically using the
```
sudo tcpdump -r example.pcap >> example
```
command to strip the headers out of the pcap file.  tcpdump isn't able to operate on files without the .pcap extension.

I set tcpdump to run for 45 seconds, just to be on the safe side as I've noticed times for fast.com
can vary between 15-35 seconds.

#### Adding a Cron Job
When trying this, it seems tcpdump needs sudo access, so I add the Cron Job to the sudo user.

```
sudo crontab -e
```

The first time you'll be asked to pick an editor (vim or nano, whichever you prefer).


Cron Jobs work by setting time fields in the first five parts of the config, and then the script or command to run.

```
minute hour day month weekday command
```

You can use * as a wildcard, meaning every hour, day, etc.

To run the job at the beginning of every hour, you can add the following line:

```
0 * * * * /path/to/script
```

So having 0 in the first position means the command will be run on the first minute of the hour.
You can use the '/' symbol to set time intervals instead of exact minutes, hours, etc.

i.e
```
*/15 * * * * command
```
will run the command every 15 minutes.

For our use case, if we want to run the different tools 5 minutes apart every hour, we can use 5 cron jobs, something like this:

```
0 * * * * fast-test
5 * * * * speedofme-test
10 * * * * iperf3-test
etc
```
or something similar for the different throttled link tests.

## Packet Analysis Tool 
A packet analysis tool was developed in order to assist our team with building fingerprints 
based on behavior. 

### Set up the virtual environment
```
cd /pcap_analysis_tool
mkdir venv
python3 -m venv ./venv
source venv/bin/activate
pip install -r requirements.txt
```

### If you want to separate pcaps from tcpdump text file
```
mkdir pcaps
cd /pcaps
```
### To get the best results, don't start the tcpdump until you are ready to send the test request
Make sure the capture at `enp0s8` so that packet captures are witnessing traffic before out tool classifies.
```
sudo tcpdump -i enp0s8 -w speedofme.pcap
```

### Start the measurement tool on VM1

### Terminate the capture
```
^C
```

### Output after termination
```
418847 packets captured
418868 packets received by filter
0 packets dropped by kernel
```

### Read only the headers into a text file
```
sudo tcpdump -r speedofme.pcap >> ../speedofme
```

### Now parse the pcaps
The outputs will be in MBit/sec. 
```
cd ..
python3 parse_packets.py speedofme
```
### Inspect the counts file
The original plan was to use bytes for the hash keys but c++ implementation for byte hash keys will be 
very involved to set up. Using string instead. Using 0, 1, 2 to avoid negative numbers. 
0 is decrease, 1 is constant, 2 is increase. 
```
"222222020220022210112001111"
```
