import copy
import os
import re
import sys

import matplotlib.pyplot as plt
import matplotlib
import pandas as pd
import seaborn as sns

matplotlib.use("QtAgg")

HEADER_REGEX = r"^(\d)+:(.)+, length (\d+)(.)*$"
TIME_REGEX = r"(\d)+:(\d)+:(\d)+.(\d)+ "

# instead of -1, 0, 1 use 0, 1, 2 so that we can use fast matching on bytes
DECREASE = 0
CONSTANT = 1
INCREASE = 2

THRESHOLD = 15


def parse_packets(filepath: str) -> dict:
    """
    Method to parse pcap data from tcpdump files.
    :param filepath: str of the pcap filepath to read in
    :return:
    """
    parsed_packet_data = {}
    packet_num = -1
    with open(filepath, "r") as file:
        for line in file:
            if re.search(HEADER_REGEX, line):
                # Don't count the ARPs
                if line.find("ARP") != -1:
                    continue

                packet_num += 1
                parsed_packet_data[packet_num] = {}

                time_segment = re.search(TIME_REGEX, line).group()
                hours, minutes, fractional_seconds = time_segment.split(":")
                seconds = int(fractional_seconds.split(".")[0])

                # Only track at the seconds resolution
                hours = int(hours)
                minutes = int(minutes)
                seconds = int(seconds)

                total_time = hours * 60 * 60 + minutes * 60 + seconds
                parsed_packet_data[packet_num]["time_elapsed"] = total_time
                try:
                    m_bits = int(line.split("length ")[1]) * 8 / 1000000
                except ValueError:
                    # Handle case for:
                    # win 64240, length 433: HTTP: POST / HTTP/1.1
                    try:
                        m_bits = int(line.split("length")[1].split(":")[0].strip()) * 8 / 1000000
                    except ValueError:
                        # Handle case:
                        # length 78 5043 2/0/0
                        m_bits = int(line.split("length")[1].strip().split(" ")[0]) * 8 / 1000000

                parsed_packet_data[packet_num]["m_bits"] = m_bits

    return parsed_packet_data


def get_stats(parsed_data_map: dict, counter: int):

    num_entries = len(parsed_data_map)
    seconds_arr = [0] * num_entries
    mb_arr = [0] * num_entries

    baseline_timestamp = parsed_data_map[0]["time_elapsed"]

    # Parse the packets in sorted order of keys, keys will be packet number arrival
    for i in sorted(parsed_data_map.keys()):
        seconds_arr[i] = parsed_data_map[i]["time_elapsed"] - baseline_timestamp
        mb_arr[i] = parsed_data_map[i]["m_bits"]

    data_frame = pd.DataFrame(data=seconds_arr, columns=["seconds"])
    data_frame["m_bits"] = mb_arr

    data_frame = data_frame.groupby("seconds")["m_bits"].sum().to_frame(name="m_bits").reset_index()

    # track the increase, decrease, constant
    num_entries = len(data_frame)
    slopes = [""] * num_entries
    slopes[0] = INCREASE

    for i in range(1, num_entries):
        prev_count = data_frame["m_bits"][i - 1]
        curr_count = data_frame["m_bits"][i]

        slopes[i] = CONSTANT
        diff = curr_count - prev_count
        meets_threshold = abs(diff) > THRESHOLD
        if diff > 0 and meets_threshold:
            slopes[i] = INCREASE
        elif diff < 0 and meets_threshold:
            slopes[i] = DECREASE

    data_frame["slopes"] = slopes
    plt.figure(figsize=(20, 10))
    plt.title("MBits/sec for %s" % filepath, fontweight="bold", fontsize=20)
    sns.lineplot(x="seconds", y="m_bits", data=data_frame, linewidth=3)

    decreases = data_frame[data_frame["slopes"] == DECREASE]
    increases = data_frame[data_frame["slopes"] == INCREASE]
    sns.scatterplot(x="seconds", y="m_bits", data=decreases, s=150, label="decreases")
    sns.scatterplot(x="seconds", y="m_bits", data=increases, s=150, label="increases")
    plt.xlabel("Seconds", fontweight="bold", fontsize=15)
    plt.ylabel("MBits/second", fontweight="bold", fontsize=15)

    # If plt.show() is not suppressed, plt.savefig() does not correctly save the figure.
    # plt.show()
    
    if not os.path.exists("figures"):
        try:
            os.makedirs("figures", exist_ok=True)
        except OSError:
            print("Error creating figures directory.")
        
    plt.savefig("./figures/%s_Sample_%d.png" % (filepath.split("/")[-1], counter))

    slopes_list = list(data_frame["slopes"])
    line = "\""
    for i in range(len(slopes_list)):
        line += "%d" % slopes_list[i]
        if i == len(slopes_list) - 1:
            line += "\"\n"

    with open("counts", "a") as f:
        f.write(line)
        f.close()


def separate_pcap_groups(parsed_packets_map: dict) -> list:
    pcap_groups = []
    pause = 5
    last_time = parsed_packets_map[0]["time_elapsed"]
    curr_group = {}
    start_packet_number = 0

    for packet_num, packet_info in parsed_packets_map.items():
        curr_time_diff = packet_info["time_elapsed"] - last_time
        if curr_time_diff >= pause:
            # We found a packet that is past our pause time. Save the current grouping
            pcap_groups.append(copy.deepcopy(curr_group))

            # Create a new group
            curr_group = {}

            # So that relative packet number can be tracked for the new group
            start_packet_number = packet_num

        relative_packet_num = packet_num - start_packet_number
        curr_group[relative_packet_num] = packet_info
        last_time = packet_info["time_elapsed"]

    # Add the last grouping to the list
    pcap_groups.append(copy.deepcopy(curr_group))
    return pcap_groups


if __name__ == "__main__":
    filepath = sys.argv[1]
    parsed_packets = parse_packets(filepath=filepath)

    get_stats(parsed_data_map=parsed_packets, counter=1)

    # The code below was for packet grouping
    """
    # These subroutines for parsing groups will not work when NAT is online since
    # random TCP requests will be made by the system (i.e. services, updates, browser, etc.)
    # However, these should have negligible effect on the MBits/sec. 
        
    packet_groupings = separate_pcap_groups(parsed_packets_map=parsed_packets)

    counter = 1
    for packet_group in packet_groupings:
        get_stats(parsed_data_map=packet_group, counter=counter)
        counter += 1
    """
