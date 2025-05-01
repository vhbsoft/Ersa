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
CLICK_DECLS
using namespace std;



Classifier::Classifier()
{
    /* Testing Definitions: Logging only portions of the behaviors we believe match a measurement tool. When building
     * this library, we will need to tune the MBits/sec threshold for each of the tools and adjust the definitions as
     * needed. While the map's size may be large, we can leverage O(1) lookup for fast classification vs. costly
     * searches for subset strings. Map<String, bool> is used since unordered_set requires iterator management.
     */
    behavior_definitions["222210"] = true;
    behavior_definitions["222210202200"] = true;
    behavior_definitions["2222102022001220"] = true;
    behavior_definitions["2222102022001220202100"] = true;
    behavior_definitions["2222102022001220202100202010"] = true;
    behavior_definitions["222210202200122020210020201022"] = true;
    behavior_definitions["222210202200122020210020201022000"] = true;
    behavior_definitions["22221020220012202021002020102200011"] = true;

    /* iPerf3 */
    behavior_definitions["222211111111111111111111111111111111000"] = true;

    /* fast.com */

    /* speedof.me */

    /* speedcheck.org */

    /* speedtest.net */

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
    string destination_port = to_string((int)(*(p->data() + 36) | *(p->data() + 37) << 8));

    string key_1 = "";
    string key_2 = "";

    key_1.append(source_ip_string).append(":").append(destination_ip_string);
    key_2.append(destination_ip_string).append(":").append(source_ip_string);

    // cout << "size of is_Flagged: " << is_Flagged.size() << "\n";

    /* Add or update entries to is_flagged */
    auto entry = is_Flagged.find(key_1);
    if (entry == is_Flagged.end()){
        // cout << "key does not exist, adding to map\n" << "\n";
        comm_entity_stat new_stat;
        new_stat.stat = REGULAR;
        new_stat.counter = 1;
        new_stat.isFlagged = false;
        is_Flagged[key_1] = new_stat;
    } else {
        // entry->counter = entry->counter + 1;
        // cout << "key does exist: " << entry->second.isFlagged << "\n";
        if (entry->second.isFlagged){ // this condition is not triggering because isFlagged is false
            // cout << "key pushed to unthrottled";
            output(1).push(p);
            // if it's a fin-ack, set isFlagged to false
            if (is_tcp && *(p->data() + 47) == 0x11){ 
                entry->second.isFlagged = false;
            }
            return;
        }
        // cout << "did not go into if loop" << "\n";
    }

    /* Do the same for the reverse direction */
    auto entry_2 = is_Flagged.find(key_2);
    if (entry_2 == is_Flagged.end()){
        comm_entity_stat new_stat;
        new_stat.stat = REGULAR;
        new_stat.isFlagged = false;
        is_Flagged[key_2] = new_stat;
    } else {
        if (entry_2->second.isFlagged){
            output(1).push(p);
            if (is_tcp && *(p->data() + 47) == 0x11){
                entry_2->second.isFlagged = false;
            }
            return;
        }
    }

    long double packet_m_bits = (p->length() * 8) / 1000000.000000;
    time_burst_map[curr_time] = time_burst_map[curr_time] + packet_m_bits;
    // cout << "Current MBits/Sec:  " << time_burst_map[curr_time] << endl;

    if (time_burst_map[curr_time] > max_m_bits_per_second) {
        max_m_bits_per_second = time_burst_map[curr_time];
    }

    /* Track slower traffic through the router to help estimate the end of testing or sending to THROTTLED
     * link since the data transfer requirements won't likely need to reach the maximum link capacity.
     */
    if (time_burst_map[curr_time] < 1.0) {
        low_burst_rate_counter++;
    }

    bool is_other_protocol = (!is_tcp && !is_udp) || is_dns;
    if (is_other_protocol || low_burst_rate_counter > 115) {
        /* Forward non-TCP and non-UDP packets to throttled port, these aren't measurement tools */
        if (is_other_protocol) {
            // cout << "OTHER PROTOCOL SENT TO THROTTLED LINK" << endl;
        } else {
            /* If the measurement tools send packets in low bursts, send them to the THROTTLED link since
             * they care be serviced at a lower rate anyways.
             */
            // cout << "LOW BURST COUNTER" << low_burst_rate_counter << endl;
            low_burst_rate_counter = 0;
        }
        output(0).push(p);
        return;
    }

    /* Allow some time to pass for individual packets to be evaluated again */
    if ((curr_time - start_burst) > observation_threshold) {
        is_measurement_tool_packet_train = false;
        low_burst_rate_counter = 0;
        start_burst = 0;
    }

    if (is_measurement_tool_packet_train) {
        output(1).push(p);
        return;
    }

    /* We know traffic is TCP or UDP now, extract TLS hellos if any */
    if (is_tcp && (p->length() > 56) && *(p->data() + 55) == 0x03 &&
        ((*(p->data() + 56) == 0x00) || (*(p->data() + 56) == 0x01) || (*(p->data() + 56) == 0x02))) {

        /* Acquire the session ID from the TLS packet */
        string session_id = get_tls_session_id(p);

        cout << "TLS SESSION ID:  " << session_id << endl;

        int tls_length = *(p->data() + 57) | *(p->data() + 58) << 8;
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
                memmem(p->data() + 57, tls_length, speed_test_net_1, 17) != nullptr ||
                memmem(p->data() + 57, tls_length, speed_test_net_2, 11) != nullptr ||
                memmem(p->data() + 57, tls_length, speed_test_net_3, 18) != nullptr;
        // cout << "before finding client hello \n";
        /* Found client hello, send all the packet through to the un-throttled link */
        if (is_measurement_tool_hello) {
            /* Print statement and sleep to inspect each measurement tool; suppress comments for performance */
            // cout << "!! FOUND CLIENT HELLO !!" << endl;
            // usleep(5000000);
        //    cout << "before changing isFlagged: " << entry->second.isFlagged << "\n";
            entry->second.isFlagged = true;
        //    cout << "after changing isFlagged: " << entry->second.isFlagged << "\n";
            output(1).push(p);
            is_measurement_tool_packet_train = true;
            if (start_burst == 0) {
                start_burst = curr_time;
            }
            return;
        }
    }

    /* PLACEHOLDER: Last attempt to classify packet traffic based on behavior in library of profiles */
    bool is_measurement_tool_behavior = false;
    /* TODO: is_flagged counters will build the behavior string during live packet trains */
    string curr_behavior = "WONT MATCH ANYTHING";
    try {
        is_measurement_tool_behavior = behavior_definitions[curr_behavior];
    } catch (const::out_of_range& exception) {
        /* Do nothing */
    }
    if (is_measurement_tool_behavior) {
        /* Print statement for inspection only; suppressed for performance */
        // cout << "!! Found a measurement tool by behavior !!" << endl; */

        output(1).push(p);
        is_measurement_tool_packet_train = true;
        return;
    }

    /* Pushing all other packets to Link 0 */
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

CLICK_ENDDECLS
ELEMENT_REQUIRES(AlignmentInfo Classification)
EXPORT_ELEMENT(Classifier)
ELEMENT_MT_SAFE(Classifier)
