/*
 * This file is submitted for University of San Francisco CS-686 Project and extends the existing code with custom
 * implementation to service the fast classification objectives of the course project. We preserve the copyright
 * information from the original file in accordance with the license agreement and we submit this work for research
 * purposes only.
 *
 * classifier.{cc,hh} -- element is a generic classifier
 * Eddie Kohler
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
 * Copyright (c) 2000 Mazu Networks, Inc.
 * Copyright (c) 2008 Regents of the University of California
 * Copyright (c) 2010 Meraki, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include "classifier.hh"
#include <click/glue.hh>
#include <click/error.hh>
#include <click/confparse.hh>
#include <click/straccum.hh>
#if !HAVE_INDIFFERENT_ALIGNMENT
#include <click/router.hh>
#endif
#include <click/standard/alignmentinfo.hh>
#include <ostream>
#include <iostream>
#include <unistd.h>
#include <stdexcept>
#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include <cstdlib>

CLICK_DECLS
using namespace std;


unordered_map<string, bool> Classifier::is_flagged_ips;

Classifier::Classifier()
{
    /* Testing Definitions: Logging only portions of the behaviors we believe match a measurement tool. When building
     * this library, we will need to tune the MBits/sec threshold for each of the tools and adjust the definitions as
     * needed. While the map's size may be large, we can leverage O(1) lookup for fast classification vs. costly
     * searches for subset strings. Map<String, bool> is used since unordered_set requires iterator management.
     */

    /* iPerf3 */
    /* Statis IP allowed us to classify on external network test. These behaviors allows us to classify on internal
     * network test where there are not IP identifiers only an abstracted behavior.
     */
    behavior_definitions["20111"] = true;
    behavior_definitions["201111111"] = true;
    behavior_definitions["20111111111"] = true;
    behavior_definitions["20111111111001"] = true;

    behavior_definitions["21111"] = true;
    behavior_definitions["211111"] = true;
    behavior_definitions["2111111"] = true;
    behavior_definitions["21111111"] = true;
    behavior_definitions["211111111"] = true;
    behavior_definitions["2111111111"] = true;
    behavior_definitions["21111111111"] = true;
    behavior_definitions["211111111111"] = true;
    behavior_definitions["2111111111111110"] = true;

    /* fast.com */
    behavior_definitions["222121"] = true;
    behavior_definitions["2221212100"] = true;
    behavior_definitions["222121210021"] = true;
    behavior_definitions["2221212100212"] = true;
    behavior_definitions["222121210021211"] = true;
    behavior_definitions["2221212100212112"] = true;
    behavior_definitions["22212121002121121"] = true;

    /* speedof.me */

    /* speedcheck.org */

    /* speedtest.net */
    behavior_definitions["21011"] = true;
    behavior_definitions["2101100"] = true;
    behavior_definitions["2101100012"] = true;
    behavior_definitions["2101100012111"] = true;
    behavior_definitions["2101100012111111"] = true;
    behavior_definitions["21011000121111110"] = true;

    behavior_definitions["21011"] = true;
    behavior_definitions["21011000"] = true;
    behavior_definitions["210110001"] = true;
    behavior_definitions["2101100012"] = true;
    behavior_definitions["21011000121"] = true;
    behavior_definitions["210110001211"] = true;
    behavior_definitions["2101100012111"] = true;
    behavior_definitions["21011000121111"] = true;
    behavior_definitions["210110001211111"] = true;
    behavior_definitions["2101100012111111"] = true;

    behavior_definitions["21111"] = true;
    behavior_definitions["211111"] = true;
    behavior_definitions["2111112"] = true;
    behavior_definitions["21111120"] = true;
    behavior_definitions["211111200"] = true;
    behavior_definitions["2111112000"] = true;
    behavior_definitions["21111120001"] = true;
    behavior_definitions["211111200011"] = true;
    behavior_definitions["2111112000111"] = true;

    behavior_definitions["2000"] = true;
    behavior_definitions["20001"] = true;
    behavior_definitions["200011"] = true;
    behavior_definitions["2000110"] = true;
    behavior_definitions["20001100"] = true;
    behavior_definitions["200011001"] = true;
    behavior_definitions["2000110012"] = true;
    behavior_definitions["20001100120"] = true;
    behavior_definitions["200011001201"] = true;
    behavior_definitions["2000110012011"] = true;
    behavior_definitions["20001100120110"] = true;

    behavior_definitions["20211"] = true;
    behavior_definitions["202111"] = true;
    behavior_definitions["2021112"] = true;
    behavior_definitions["20211121"] = true;
    behavior_definitions["202111211"] = true;
    behavior_definitions["2021112111"] = true;
    behavior_definitions["20211121110"] = true;
    behavior_definitions["202111211101"] = true;
    behavior_definitions["2021112111011"] = true;
    behavior_definitions["20211121110111"] = true;
    behavior_definitions["202111211101110"] = true;
    behavior_definitions["2021112111011100"] = true;

    behavior_definitions["20111"] = true;
    behavior_definitions["201112"] = true;
    behavior_definitions["2011121"] = true;
    behavior_definitions["20111211"] = true;
    behavior_definitions["201112112"] = true;
    behavior_definitions["2011121121"] = true;
    behavior_definitions["20111211210"] = true;
    behavior_definitions["201112112102"] = true;
    behavior_definitions["2011121121021"] = true;
    behavior_definitions["20111211210210"] = true;
    behavior_definitions["201112112102100"] = true;
    behavior_definitions["2011121121021001"] = true;
    behavior_definitions["20111211210210011"] = true;
    behavior_definitions["201112112102100111"] = true;
    behavior_definitions["2011121121021001111"] = true;
    behavior_definitions["20111211210210011111"] = true;
    behavior_definitions["201112112102100111111"] = true;
    behavior_definitions["2011121121021001111111"] = true;
    behavior_definitions["20111211210210011111111"] = true;
    behavior_definitions["201112112102100111111111"] = true;
    behavior_definitions["2011121121021001111111110"] = true;
}

Classification::Wordwise::Program
Classifier::empty_program(ErrorHandler *errh) const
{
    // set align offset
    int c, o;
    if (AlignmentInfo::query(this, 0, c, o) && c >= 4)
        // want 'data - _align_offset' aligned at 4/(o%4)
        o = (4 - (o % 4)) % 4;
    else {
#if !HAVE_INDIFFERENT_ALIGNMENT
        if (errh) {
            errh->warning("alignment unknown, but machine is sensitive to alignment");
            void *&x = router()->force_attachment("Classifier alignment warning");
            if (!x) {
                x = (void *) this;
                errh->message("(%s must be told how its input packets are aligned in memory.\n"
                              "Fix this error either by passing your configuration through click-align,\n"
                              "or by providing explicit AlignmentInfo. I am assuming the equivalent\n"
                              "of %<AlignmentInfo(%s 4 0)%>.)", class_name(), name().c_str());
            }
        }
#else
        (void) errh;
#endif
        o = 0;
    }
    return Classification::Wordwise::Program(o);
}

static void
update_value_mask(int c, int shift, int &value, int &mask)
{
    int v = 0, m = 0xF;
    if (c == '?')
        v = m = 0;
    else if (c >= '0' && c <= '9')
        v = c - '0';
    else if (c >= 'A' && c <= 'F')
        v = c - 'A' + 10;
    else if (c >= 'a' && c <= 'f')
        v = c - 'a' + 10;
    value |= (v << shift);
    mask |= (m << shift);
}

void
Classifier::parse_program(Classification::Wordwise::Program &prog,
                          Vector<String> &conf, ErrorHandler *errh)
{
    Vector<int> tree = prog.init_subtree();
    prog.start_subtree(tree);

    for (int slot = 0; slot < conf.size(); slot++) {
        int i = 0;
        int len = conf[slot].length();
        const char *s = conf[slot].data();
        bool empty = true;

        prog.start_subtree(tree);

        if (s[0] == '-' && len == 1)
            // slot accepting everything
            i = 1;

        while (i < len) {

            while (i < len && isspace((unsigned char) s[i]))
                i++;
            if (i >= len)
                break;

            // negated?
            bool negated = false;
            if (s[i] == '!') {
                negated = true;
                i++;
                while (i < len && isspace((unsigned char) s[i]))
                    i++;
            }

            if (i >= len || !isdigit((unsigned char) s[i])) {
                errh->error("pattern %d: expected a digit", slot);
                break;
            }

            // read offset
            int offset = 0;
            while (i < len && isdigit((unsigned char) s[i])) {
                offset *= 10;
                offset += s[i] - '0';
                i++;
            }

            if (i >= len || s[i] != '/') {
                errh->error("pattern %d: expected %</%>", slot);
                break;
            }
            i++;

            // scan past value
            int value_pos = i;
            while (i < len && (isxdigit((unsigned char) s[i]) || s[i] == '?'))
                i++;
            int value_end = i;

            // scan past mask
            int mask_pos = -1;
            int mask_end = -1;
            if (i < len && s[i] == '%') {
                i++;
                mask_pos = i;
                while (i < len && (isxdigit((unsigned char) s[i]) || s[i] == '?'))
                    i++;
                mask_end = i;
            }

            // check lengths
            if (value_end - value_pos < 2) {
                errh->error("pattern %d: value has less than 2 hex digits", slot);
                value_end = value_pos;
                mask_end = mask_pos;
            }
            if ((value_end - value_pos) % 2 != 0) {
                errh->error("pattern %d: value has odd number of hex digits", slot);
                value_end--;
                mask_end--;
            }
            if (mask_pos >= 0 && (mask_end - mask_pos) != (value_end - value_pos)) {
                bool too_many = (mask_end - mask_pos) > (value_end - value_pos);
                errh->error("pattern %d: mask has too %s hex digits", slot,
                            (too_many ? "many" : "few"));
                if (too_many)
                    mask_end = mask_pos + value_end - value_pos;
                else
                    value_end = value_pos + mask_end - mask_pos;
            }

            // add values to exprs
            prog.start_subtree(tree);

            bool first = true;
            offset += prog.align_offset();
            while (value_pos < value_end) {
                int v = 0, m = 0;
                update_value_mask(s[value_pos], 4, v, m);
                update_value_mask(s[value_pos+1], 0, v, m);
                value_pos += 2;
                if (mask_pos >= 0) {
                    int mv = 0, mm = 0;
                    update_value_mask(s[mask_pos], 4, mv, mm);
                    update_value_mask(s[mask_pos+1], 0, mv, mm);
                    mask_pos += 2;
                    m = m & mv & mm;
                }
                if (first || offset % 4 == 0) {
                    prog.add_insn(tree, (offset / 4) * 4, 0, 0);
                    first = empty = false;
                }
                prog.back().mask.c[offset % 4] = m;
                prog.back().value.c[offset % 4] = v & m;
                offset++;
            }

            // combine with "and"
            prog.finish_subtree(tree, Classification::c_and);

            if (negated)
                prog.negate_subtree(tree);
        }

        // add fake expr if required
        if (empty)
            prog.add_insn(tree, 0, 0, 0);

        prog.finish_subtree(tree, Classification::c_and,
                            -slot, Classification::j_failure);
    }

    prog.finish_subtree(tree, Classification::c_or, Classification::j_never, Classification::j_never);

    // click_chatter("%s", prog.unparse().c_str());
    prog.optimize(0, 0, Classification::offset_max);
    // click_chatter("%s", prog.unparse().c_str());
}

int
Classifier::configure(Vector<String> &conf, ErrorHandler *errh)
{
    if (conf.size() != noutputs())
        return errh->error("need %d arguments, one per output port", noutputs());

    Classification::Wordwise::Program prog = empty_program(errh);
    parse_program(prog, conf, errh);

    if (!errh->nerrors()) {
        prog.warn_unused_outputs(noutputs(), errh);
        _prog = prog;
        return 0;
    } else
        return -1;
}

String
Classifier::program_string(Element *element, void *)
{
    Classifier *c = static_cast<Classifier *>(element);
    return c->_prog.unparse();
}

void
Classifier::add_handlers()
{
    add_read_handler("program", Classifier::program_string, 0, Handler::CALM);
}

void
Classifier::push(int, Packet *p)
{
    /* Setup tracking for packet train bursts */
    long curr_time = static_cast<long>(time(0));
    try {
        time_burst_map[curr_time];
    } catch (const::out_of_range& exception) {
        time_burst_map[curr_time] = 0.000000;
    }

    /* Find TCP and UDP packets*/
    bool is_tcp = *(p->data() + 12) == 0x08 && *(p->data() + 13) == 0x00 && *(p->data() + 23) == 0x06;
    bool is_udp = *(p->data() + 12) == 0x08 && *(p->data() + 13) == 0x00 && *(p->data() + 23) == 0x11;
    bool is_dns = (is_tcp || is_udp) && *(p->data() + 44) == 0x01 && *(p->data() + 45) == 0x00;

    // Acquire the Source, Destination IP, and Destination Port
    char32_t source_ip[] = {(char32_t) *(p->data() + 26), (char32_t) *(p->data() + 27), (char32_t) *(p->data() + 28), (char32_t) *(p->data() + 29)};
    char32_t destination_ip[] = {(char32_t) *(p->data() + 30), (char32_t) *(p->data() + 31), (char32_t) *(p->data() + 32), (char32_t) *(p->data() + 33)};
    string source_ip_string = get_ip_string(&source_ip[0], 4);
    string destination_ip_string = get_ip_string(&destination_ip[0], 4);

    /* Obtain port number with bit-shifting */
    // string destination_port = to_string((int)(*(p->data() + 36) | *(p->data() + 37) << 8));

    string suspected_ips_1 = "";
    string suspected_ips_2 = "";

    suspected_ips_1.append(source_ip_string).append(":").append(destination_ip_string);
    suspected_ips_2.append(destination_ip_string).append(":").append(source_ip_string);

    long double packet_m_bits = (p->length() * 8) / 1000000.000000;
    time_burst_map[curr_time] = time_burst_map[curr_time] + packet_m_bits;
    // cout << "Current MBits/Sec:  " << time_burst_map[curr_time] << endl;

    if (time_burst_map[curr_time] > max_m_bits_per_second) {
        max_m_bits_per_second = time_burst_map[curr_time];
    }

    if(time_burst_map[curr_time] > 0) {
        int burst_difference = time_burst_map[curr_time] - time_burst_map[curr_time - 1];
        bool increased_rate = abs(burst_difference) > burst_threshold && burst_difference > 0;
        bool decreased_rate = abs(burst_difference) < burst_threshold && burst_difference < 0;
        /* 0: decrease, 1: constant, 2: increase */
        int curr_behavior = 1;
        if (increased_rate) {
            curr_behavior = 2;
            if (found_first_burst) {
                current_behavior_map.clear(); /* Track a new behavior */
                found_first_burst = false;
            }
        } else if (decreased_rate) {
            curr_behavior = 0;
        }
        current_behavior_map[curr_time] = curr_behavior;
    }

    string curr_behavior = get_current_behavior_string();
    cout << "Current Behavior Abstraction: " << curr_behavior << endl;

    /* Track slower traffic through the router to help estimate the end of testing or sending to THROTTLED
     * link since the data transfer requirements won't likely need to reach the maximum link capacity.
     */
    if (time_burst_map[curr_time] < 1.0) {
        low_burst_rate_counter++;
    }

    /* The reliable iPerf3 public server that can be accessed consistently on the internet is located
     * at iperf3.moji.fr. This endpoint has the static IP address of 145.147.210.189.
     * Other servers like: iperf.scottlinux.com and iperf.he.net are offline and always return an
     * error (Unable to send control message: Bad file descriptor). Our project uses 145.147.210.189
     * to flag this bandwidth measurement tool for testing purposes on an external network for consistent
     * testing controls with the other bandwidth measurement tools hosted on external networks.
     */
    if (source_ip_string == "45.147.210.189" || destination_ip_string == "45.147.210.189") {
        unthrottled_counter++;
        is_flagged_ips[suspected_ips_1] = true;
        is_flagged_ips[suspected_ips_2] = true;
        output(1).push(p);
        return;
    }

    bool is_other_protocol = (!is_tcp && !is_udp) || is_dns;
    if (is_other_protocol || low_burst_rate_counter > 115) {
        /* Forward non-TCP and non-UDP packets to throttled port, these aren't measurement tools */
        if (!is_other_protocol) {
            low_burst_rate_counter = 0;
        }
        throttled_counter++;
        output(0).push(p);
        return;
    }

    /* Allow some time to pass for individual packets to be evaluated again */
    if (start_burst > 0 && ((curr_time - start_burst) > observation_threshold) ||
        current_behavior_map.size() > observation_threshold) {
        /* So that we can track the new behavior next time. It will be toggled when a new behavior is tracked */
        found_first_burst = true;
        low_burst_rate_counter = 0;
        start_burst = 0;
        print_statistics();
        is_flagged_ips.clear();
        throttled_counter = 0;
        unthrottled_counter = 0;
        current_behavior_map.clear();
        cout << "\t\t\t\t\t\t *** RESET OBSERVATION STATE ***" << endl;
    }

    if (is_flagged_ips[suspected_ips_1] || is_flagged_ips[suspected_ips_2]) {
        output(1).push(p);
        unthrottled_counter++;
        return;
    }

    /* We know traffic is TCP or UDP now, extract TLS hellos if any */
    if (is_tcp && (p->length() > 56) && *(p->data() + 55) == 0x03 &&
        ((*(p->data() + 56) == 0x00) || (*(p->data() + 56) == 0x01) || (*(p->data() + 56) == 0x02))) {

        int tls_length = *(p->data() + 57) | *(p->data() + 58) << 8;
        if (p->length() < tls_length) {
            tls_length = p->length();
        }

        bool is_measurement_tool_hello =
                memmem(p->data() + 57, tls_length, speed_of_me_1, 14) != nullptr ||
                memmem(p->data() + 57, tls_length, speed_of_me_2, 10) != nullptr ||
                memmem(p->data() + 57, tls_length, fast_com_1, 8) != nullptr ||
                memmem(p->data() + 57, tls_length, fast_com_2, 12) != nullptr ||
                memmem(p->data() + 57, tls_length, fast_com_3, 23) != nullptr ||
                memmem(p->data() + 57, tls_length, fast_com_4, 17) != nullptr ||
                memmem(p->data() + 57, tls_length, speed_check_org_1, 18) != nullptr ||
                memmem(p->data() + 57, tls_length, speed_check_org_2, 18) != nullptr ||
                memmem(p->data() + 57, tls_length, speed_check_org_3, 17) != nullptr ||
                memmem(p->data() + 57, tls_length, speed_check_org_4, 15) != nullptr ||
                memmem(p->data() + 57, tls_length, speed_test_net_1, 17) != nullptr ||
                memmem(p->data() + 57, tls_length, speed_test_net_2, 11) != nullptr ||
                memmem(p->data() + 57, tls_length, speed_test_net_3, 18) != nullptr ||
                memmem(p->data() + 57, tls_length, speed_test_net_4, 24) != nullptr ||
                memmem(p->data() + 57, tls_length, speed_test_net_5, 29) != nullptr ||
                memmem(p->data() + 57, tls_length, speed_test_net_6, 21) != nullptr ||
                memmem(p->data() + 57, tls_length, speed_test_net_7, 14) != nullptr ||
                memmem(p->data() + 57, tls_length, speed_test_net_8, 20) != nullptr ||
                memmem(p->data() + 57, tls_length, speed_test_net_9, 19) != nullptr ||
                memmem(p->data() + 57, tls_length, speed_test_net_10, 20) != nullptr;

        /* Found client hello, send all the packet through to the un-throttled link */
        if (is_measurement_tool_hello) {
            unthrottled_counter++;

            is_flagged_ips[suspected_ips_1] = true;
            is_flagged_ips[suspected_ips_2] = true;
            if (start_burst == 0) {
                start_burst = curr_time;
            }
            output(1).push(p);
            return;
        }
    }

    /* Last-chance classification on behavior */
    auto iterator = behavior_definitions.find(curr_behavior);
    if (iterator != behavior_definitions.end()) {
        /* We found a behavior in our library of behavior definitions */
        unthrottled_counter++;

        is_flagged_ips[suspected_ips_1] = true;
        is_flagged_ips[suspected_ips_2] = true;
        if (start_burst == 0) {
            start_burst = curr_time;
        }
        output(1).push(p);
        return;
    }


    /* Pushing all other packets to Link 0 */
    throttled_counter++;
    output(0).push(p);
}

string
Classifier::get_ip_string(char32_t* curr_chars, size_t length) {
    /* A helper method to acquire the ip strings like: 12.123.456.10 */
    string curr_string;
    for (size_t i = 0; i < length; i++) {
        curr_string.append(to_string(*(curr_chars + i)));
        if (i < length - 1) {
            curr_string.append(".");
        }
    }
    return curr_string;
}

string
Classifier::get_tls_session_id(Packet* p) {
    /* A helper method to acquire the TLS session id */
    string curr_string;
    int session_id_length = static_cast<int>(*(p->data() + 97));
    char* curr_chars = new char[session_id_length]();

    /* Wrangle the unsigned chars into a char[] so that we can make them string.
     * string is needed for hashing into the unordered map.
     */
    for (int i = 0; i < session_id_length; i++) {
        const unsigned char curr_char = (unsigned char) *(p->data() + (i + 98));
        curr_chars[i] = curr_char;
    }

    for (int i = 0; i < session_id_length; i++) {
        curr_string.append(to_string(*(curr_chars + i)));
    }

    return curr_string;
}

void
Classifier::print_statistics() {
    cout << "STATISTICS" << endl;
    cout << "# THROTTLED PACKETS: " << throttled_counter << endl;
    cout << "# UNTHROTTLED PACKETS: " << unthrottled_counter << endl;
    cout << "BANDWIDTH MEASUREMENT TOOL IPS: " << endl;
    for (auto entry = is_flagged_ips.begin(); entry != is_flagged_ips.end(); entry++) {
        cout << entry->first << endl;
    }

    long curr_time = static_cast<long>(time(0));

    ofstream stats_file;
    stats_file.open("stats_file" + std::to_string(curr_time) + ".txt" );

    stats_file << "# THROTTLED PACKETS: " << throttled_counter << endl;
    stats_file << "# UNTHROTTLED PACKETS: " << unthrottled_counter << endl;
    stats_file << "BANDWIDTH MEASUREMENT TOOL IPS: " << endl;
    for (auto entry = is_flagged_ips.begin(); entry != is_flagged_ips.end(); entry++) {
        stats_file << entry->first << endl;
    }
    stats_file.close();
}

string
Classifier::get_current_behavior_string() {
    string curr_behavior_string = "";
    for (auto entry = current_behavior_map.begin(); entry != current_behavior_map.end(); entry++) {
        curr_behavior_string.append(std::to_string(entry->second));
    }
    return curr_behavior_string;
}

CLICK_ENDDECLS
ELEMENT_REQUIRES(AlignmentInfo Classification)
EXPORT_ELEMENT(Classifier)
ELEMENT_MT_SAFE(Classifier)
