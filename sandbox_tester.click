/* Our classifier takes all packets but needs 2 inputs since we configure 2 outputs 
 * where output 0 is for the THROTTLED link and output 1 is for the UNTHROTTLED link.
 * 50 Mbits/sec = 6250000 Bytes/sec
 * 40 Mbits/sec = 5000000 Bytes/sec
 * 30 Mbits/sec = 3750000 Bytes/sec
 * 20 Mbits/sec = 2500000 Bytes/sec
 * 10 Mbits/sec = 1250000 Bytes/sec
 */
classifier :: Classifier(12/0800, -);

out_server :: ToDevice(enp0s9);
out_client :: Queue(80000000) -> ToDevice(enp0s8);
scheduler :: PrioSched -> out_server;

FromDevice(enp0s8, PROMISC true)
	-> classifier;

/* Remove the Print() after testing. Use 0 to supress packet info printed to stdout.
 * Capture packets at enp0s8 for behavior definitions before classification.
 */
classifier[0]
    -> Queue(80000000)
    -> Print("## Throttled ##", 0)
    -> BandwidthShaper(1250000B/s)
    -> [1] scheduler;

/* The un-throttled link is left fully un-throttled without a bandwidth shaper for now */
classifier[1]
    -> Queue(80000000)
    -> Print("!! Bandwidth Measurement Tool !!", 0)
    -> [0] scheduler;

/* Send anything from the server to the client */
FromDevice(enp0s9, PROMISC true) -> out_client;

