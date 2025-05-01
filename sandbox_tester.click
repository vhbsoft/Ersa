/* Our classifier takes all packets but needs 2 inputs since we configure 2 outputs 
 * where output 0 is for the THROTTLED link and output 1 is for the UNTHROTTLED link.
 * 50 Mbits/sec = 6250000 Bytes/sec
 * 40 Mbits/sec = 5000000 Bytes/sec
 * 30 Mbits/sec = 3750000 Bytes/sec
 * 20 Mbits/sec = 2500000 Bytes/sec
 * 10 Mbits/sec = 1250000 Bytes/sec
 */
classifier :: Classifier(12/0800, -);
classifier_server :: Classifier(12/0800, -);


out_server :: ToDevice(enp0s9);
out_client :: ToDevice(enp0s8);
scheduler :: PrioSched -> out_server;

scheduler_server :: PrioSched -> out_client;


FromDevice(enp0s8, PROMISC true)
	-> classifier;

/* Remove the Print() after testing. Use 0 to supress packet info printed to stdout.
 * Capture packets at enp0s9 for behavior definitions @ the maximum link capacity.
 */
classifier[0]
    -> Queue(80000000)
    -> Print("CLIENT: ## Throttled ##", 0)
    -> BandwidthShaper(1250000B/s)
    -> [1] scheduler;

classifier[1]
    -> Queue(80000000)
    -> Print("CLIENT: !! Bandwidth Measurement Tool !!", 0)
    -> [0] scheduler;

/* Send anything from the server to the client */
FromDevice(enp0s9, PROMISC true)
    -> classifier_server;

classifier_server[0]
    -> Queue(80000000)
    -> Print("\t\t\t\t\t\tSERVER: ## Throttled ##", 0)
    -> BandwidthShaper(1250000B/s)
    -> [1] scheduler_server;

classifier_server[1]
    -> Queue(80000000)
    -> Print("\t\t\t\t\t\tSERVER: !! Bandwidth Measurement Tool !!", 0)
    -> [0] scheduler_server;
