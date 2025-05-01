/*
 * This file is submitted for University of San Francisco CS-686 Project and extends the existing code with custom
 * implementation to service the fast classification objectives of the course project. We preserve the copyright
 * information from the original file in accordance with the license agreement and we submit this work for research
 * purposes only.
 */

#ifndef CLICK_CLASSIFIER_HH
#define CLICK_CLASSIFIER_HH
#include <click/element.hh>
#include <click/hashmap.hh>
#include "classification.hh"
#include <unordered_map>
#include <map>
#include <string>
CLICK_DECLS
using namespace std;

/*
 * =c
 * Classifier(pattern1, ..., patternN)
 * =s classification
 * classifies packets by contents
 * =d
 * Classifies packets. The Classifier has N outputs, each associated with the
 * corresponding pattern from the configuration string.
 * A pattern is a set of clauses, where each clause is either "offset/value"
 * or "offset/value%mask". A pattern matches if the packet has the indicated
 * value at each offset.
 *
 * The clauses in each pattern are separated
 * by spaces. A clause consists of the offset, "/", the value, and (optionally)
 * "%" and a mask. The offset is in decimal. The value and mask are in hex.
 * The length of the value is implied by the number of hex digits, which must
 * be even. "?" is also allowed as a "hex digit"; it means "don't care about
 * the value of this nibble".
 *
 * If present, the mask must have the same number of hex digits as the value.
 * The matcher will only check bits that are 1 in the mask.
 *
 * A clause may be preceded by "!", in which case the clause must NOT match
 * the packet.
 *
 * As a special case, a pattern consisting of "-" matches every packet.
 *
 * The patterns are scanned in order, and the packet is sent to the output
 * corresponding to the first matching pattern. Thus more specific patterns
 * should come before less specific ones. You will get a warning if no packet
 * could ever match a pattern. Usually, this is because an earlier pattern is
 * more general, or because your pattern is contradictory (`12/0806 12/0800').
 *
 * =n
 *
 * The IPClassifier and IPFilter elements have a friendlier syntax if you are
 * classifying IP packets.
 *
 * =e
 * For example,
 *
 *   Classifier(12/0806 20/0001,
 *              12/0806 20/0002,
 *              12/0800,
 *              -);
 *
 * creates an element with four outputs intended to process
 * Ethernet packets.
 * ARP requests are sent to output 0, ARP replies are sent to
 * output 1, IP packets to output 2, and all others to output 3.
 *
 * =h program read-only
 * Returns a human-readable definition of the program the Classifier element
 * is using to classify packets. At each step in the program, four bytes
 * of packet data are ANDed with a mask and compared against four bytes of
 * classifier pattern.
 *
 * The Classifier patterns above compile into the following program:
 *
 *   0  12/08060000%ffff0000  yes->step 1  no->step 3
 *   1  20/00010000%ffff0000  yes->[0]  no->step 2
 *   2  20/00020000%ffff0000  yes->[1]  no->[3]
 *   3  12/08000000%ffff0000  yes->[2]  no->[3]
 *   safe length 22
 *   alignment offset 0
 *
 * =a IPClassifier, IPFilter */

class Classifier : public Element { public:

    Classifier() CLICK_COLD;

    const char *class_name() const		{ return "Classifier"; }
    const char *port_count() const		{ return "1/-"; }
    const char *processing() const		{ return PUSH; }
    // this element needs AlignmentInfo, so supply the "A" flag
    const char *flags() const			{ return "A"; }
    bool can_live_reconfigure() const		{ return true; }

    int configure(Vector<String> &conf, ErrorHandler *errh) CLICK_COLD;
    void add_handlers() CLICK_COLD;

    void push(int port, Packet *);

    Classification::Wordwise::Program empty_program(ErrorHandler *errh) const;
    static void parse_program(Classification::Wordwise::Program &prog,
                              Vector<String> &conf, ErrorHandler *errh);

protected:

    Classification::Wordwise::Program _prog;

    static String program_string(Element *, void *);

    enum status { REGULAR = 0, SUSPECTED = 1, CONFIRMED = 2};
    struct comm_entity_stat {
        enum status stat;   // not used in solution 2
        int counter;        // not used currently
        bool isFlagged;
    };
    unordered_map<string, comm_entity_stat> is_Flagged;

    /* cdn.speedof.me */
    const char speed_of_me_1[14] = {0x63, 0x64, 0x6e, 0x2e, 0x73, 0x70, 0x65,
                                    0x65, 0x64, 0x6f, 0x66, 0x2e, 0x6d, 0x65};


    const char speed_of_me_2[10] = {0x73, 0x70, 0x65, 0x65, 0x64, 0x6f, 0x66,
                                    0x2e, 0x6d, 0x65};
    /* fast.com */
    const char fast_com_1[8] = {0x66, 0x61, 0x73, 0x74, 0x2e, 0x63, 0x6f, 0x6d};
    /* api.fast.com */
    const char fast_com_2[12] = {0x61, 0x70, 0x69, 0x2e, 0x66, 0x61, 0x73, 0x74,
                                 0x2e, 0x63, 0x6f, 0x6d};
    const char fast_com_3[23] = {0x69, 0x63, 0x68, 0x6e, 0x61, 0x65, 0x61, 0x2d,
                                 0x77, 0x65, 0x62, 0x2e, 0x6e, 0x65, 0x74, 0x66,
                                 0x6c, 0x69, 0x78, 0x2e, 0x63, 0x6f, 0x6d};
    const char fast_com_4[17] = {0x6f, 0x63, 0x61, 0x2e, 0x6e, 0x66, 0x6c, 0x78,
                                 0x76, 0x69, 0x64, 0x65, 0x6f, 0x2e, 0x6e, 0x65,
                                 0x74};
    /* www.speedcheck.org */
    const char speed_check_org_1[18] = {0x77, 0x77, 0x77, 0x2e, 0x73, 0x70, 0x65, 0x65,
                                        0x64, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x2e,
                                        0x6f, 0x72, 0x67};
    /* cdn.speedcheck.org */
    const char speed_check_org_2[18] = {0x63, 0x64, 0x6e, 0x2e, 0x73, 0x70, 0x65, 0x65,
                                        0x64, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x2e,
                                        0x6f, 0x72, 0x67};

    /* api.speedspot.org */
    const char speed_check_org_3[17] = {0x61, 0x70, 0x69, 0x2e, 0x73, 0x70, 0x65, 0x65,
                                        0x64, 0x73, 0x70, 0x6f, 0x74, 0x2e, 0x6f,
                                        0x72, 0x67};

    /* b1.etrality.com */
    const char speed_check_org_4[15] = {0x62, 0x31, 0x2e, 0x65, 0x74, 0x72, 0x61, 0x6c,
                                        0x69, 0x74, 0x79, 0x2e, 0x63, 0x6f, 0x6d};
    /* www.speedtest.net */
    const char speed_test_net_1[17] = {0x77, 0x77, 0x77, 0x2e, 0x73, 0x70, 0x65, 0x65,
                                       0x64, 0x74, 0x65, 0x73, 0x74, 0x2e, 0x6e,
                                       0x65, 0x74};
    /* b.cdnst.net */
    const char speed_test_net_2[11] = {0x62, 0x2e, 0x63, 0x64, 0x6e, 0x73, 0x74, 0x2e,
                                       0x6e, 0x65, 0x74};

    /* cdn.ziffstatic.com */
    const char speed_test_net_3[18] = {0x63, 0x64, 0x6e, 0x2e, 0x7a, 0x69, 0x66, 0x66,
                                       0x73, 0x74, 0x61, 0x74, 0x69, 0x63, 0x2e,
                                       0x63, 0x6f, 0x6d};

    /* www.googletagmanager.com */
    const char speed_test_net_4[24] = {0x77, 0x77, 0x77, 0x2e, 0x67, 0x6f, 0x6f, 0x67,
                                       0x6c, 0x65, 0x74, 0x61, 0x67, 0x6d, 0x61,
                                       0x6e, 0x61, 0x67, 0x65, 0x72, 0x2e, 0x63,
                                       0x6f, 0x6d};

    /* d3div1mtym39ic.cloudfront.net */
    const char speed_test_net_5[29] = {0x64, 0x33, 0x64, 0x69, 0x76, 0x31, 0x6d, 0x74,
                                       0x79, 0x6d, 0x33, 0x39, 0x69, 0x63, 0x2e,
                                       0x63, 0x6c, 0x6f, 0x75, 0x64, 0x66, 0x72,
                                       0x6f, 0x6e, 0x74, 0x2e, 0x6e, 0x65, 0x74};
    /* c.amazon-adsystem.com */
    const char speed_test_net_6[21] = {0x63, 0x2e, 0x61, 0x6d, 0x61, 0x7a, 0x6f, 0x6e,
                                       0x2d, 0x61, 0x64, 0x73, 0x79, 0x73, 0x74,
                                       0x65, 0x6d, 0x2e, 0x63, 0x6f, 0x6d};
    /* acdn.adnxs.com */
    const char speed_test_net_7[14] = {0x61, 0x63, 0x64, 0x6e, 0x2e, 0x61, 0x64, 0x6e,
                                       0x78, 0x73, 0x2e, 0x63, 0x6f, 0x6d};
    /* contextual.media.net */
    const char speed_test_net_8[20] = {0x63, 0x6f, 0x6e, 0x74, 0x65, 0x78, 0x74, 0x75,
                                       0x61, 0x6c, 0x2e, 0x6d, 0x65, 0x64, 0x69,
                                       0x61, 0x2e, 0x6e, 0x65, 0x74};
    /* image6.pubmatic.com */
    const char speed_test_net_9[19] = {0x69, 0x6d, 0x61, 0x67, 0x65, 0x36, 0x2e, 0x70,
                                       0x75, 0x62, 0x6d, 0x61, 0x74, 0x69, 0x63,
                                       0x2e, 0x63, 0x6f, 0x6d};
    /* simage4.pubmatic.com */
    const char speed_test_net_10[20] = {0x73, 0x69, 0x6d, 0x61, 0x67, 0x65, 0x34, 0x2e,
                                        0x70, 0x75, 0x62, 0x6d, 0x61, 0x74, 0x69,
                                        0x63, 0x2e, 0x63, 0x6f, 0x6d};

    /* Behavior definitions are MBits/sec packet traffic profiles that flag behavior as bandwidth measurement tools */
    unordered_map<string, bool> behavior_definitions;
    unordered_map<long, long double> time_burst_map;
    unordered_map<string, bool> session_ids;
    map<long, int> current_behavior_map;
    bool found_first_burst = true;
    int burst_threshold = 10; /* MBits/sec */
    static unordered_map<string, bool> is_flagged_ips;


    /* Observation elements */
    int low_burst_rate_counter = 0;
    long double max_m_bits_per_second = 0;
    long start_burst = 0;
    int observation_threshold = 30;

    /* Packet Counter */
    int throttled_counter = 0;
    int unthrottled_counter = 0;

    /* Helper methods */
    string get_ip_string(char32_t*, size_t);
    string get_tls_session_id(Packet*);
    string get_current_behavior_string();
    void print_statistics();
};

CLICK_ENDDECLS
#endif
