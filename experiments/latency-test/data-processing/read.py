#!/usr/bin/env python3
from scapy.all import PcapNgReader
from itertools import *
import itertools

# https://napsterinblue.github.io/notes/python/internals/itertools_sliding_window/
def sliding_window(iterable, n=2):
    iterables = itertools.tee(iterable, n)
    for iterable, num_skipped in zip(iterables, itertools.count()):
        for _ in range(num_skipped):
            next(iterable, None)
    return zip(*iterables)


# we assume the pcapng file has already been filtered by opendroneid,
# and only two specific mac addresses

sender_mac = "02:45:6d:ff:dd:dc"
relayer_mac = "02:45:6d:ff:dd:dd"


def process_pcap_file(filename):
  reader = PcapNgReader(filename)
  packets = list(reader)

  time_diffs = []

  for prev, next in sliding_window(packets, 2):
    msg_count_prev = prev["Dot11EltVendorSpecific"].info[4]
    msg_count_next = next["Dot11EltVendorSpecific"].info[4]
    if prev.addr2 == sender_mac and next.addr2 == relayer_mac and msg_count_prev == msg_count_next:
      time_diff = next.time - prev.time
      time_diffs.append(time_diff)
      # print(f"time diff {time_diff}")
  return time_diffs
  
def write_list_to_file(numbers, filename):
  f = open(filename, 'w')
  for n in numbers:
    f.write(str(n) + '\n')

for file in ["latency-test-usb-adapter-YS7-10min", "latency-test-YS7-10min"]:
  print(f"Processing {file}")
  time_diffs = process_pcap_file(f"../data/{file}.pcapng")
  write_list_to_file(time_diffs, f"../data/{file}-latencies.txt")
