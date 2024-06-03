#!/usr/bin/env python3
from scapy.all import PcapNgReader
from itertools import *
import matplotlib.pyplot as plt
import itertools
import numpy as np
from scipy.ndimage import uniform_filter1d
# https://napsterinblue.github.io/notes/python/internals/itertools_sliding_window/
def sliding_window(iterable, n=2):
    iterables = itertools.tee(iterable, n)
    for iterable, num_skipped in zip(iterables, itertools.count()):
        for _ in range(num_skipped):
            next(iterable, None)
    return zip(*iterables)


ID_drone = "EDRI-D2"
ID_fc = 'EDRI-D1'
ID_airplate_victor = "ARPLF352709570739565"
ID_airplate_mads = "ARPLF352709570738211"
ID_list = [ID_drone, ID_fc, ID_airplate_mads, ID_airplate_victor]
count_list = [0,0,0,0]

device = ID_drone
pcapfile = "../data/relay_test_"+device+"_F4relay.pcapng"
pcapfile_airplate = "../data/relay_test_"+ID_airplate_victor+"_F4relay.pcapng"
pcapfile_drone = "../data/relay_test_"+ID_drone+"_F4relay.pcapng"
# reader = PcapNgReader("../data/relay_test_"+ID_drone+".pcapng")
# Get start and stop times of the airplate and drone
reader_airplate = PcapNgReader(pcapfile_airplate)
packets_airplate = list(reader_airplate)
t_start_airplate = float(packets_airplate[0].time)
t_end_airplate = float(packets_airplate[-1].time)
reader_drone = PcapNgReader(pcapfile_drone)
packets_drone = list(reader_drone)
t_start_drone = float(packets_drone[0].time)
t_end_drone = float(packets_drone[-1].time)


reader = PcapNgReader(pcapfile)
packets = list(reader)
window_size = 150
t_start = float(packets[0].time)
t_end = float(packets[-1].time)

time_diffs_drone = []
freq_drone = []
altitude_drone = []
time_diffs_airplate = []
freq_airplate = []

print("Airplate start, stop: ",t_start_airplate," , ",t_end_airplate)
print("Drone start, stop: ",t_start_drone," , ",t_end_drone)
print("start diff",t_start_airplate-t_start_drone)


for item in sliding_window(packets_drone,window_size):
    t_span = float(item[window_size-1].time-item[0].time)
    t_avg = t_span/float(window_size)
    time_diffs_drone.append(t_avg)
    freq_drone.append(1.0/t_avg)
    alt_sum = 0
    for i in range(window_size):
        alt_sum = alt_sum + int.from_bytes(item[i]["Dot11EltVendorSpecific"].info[48:50],byteorder='little')
    altitude_drone.append(alt_sum/window_size)



for item in sliding_window(packets_airplate,window_size):
    t_span = float(item[window_size-1].time-item[0].time)
    t_avg = t_span/float(window_size)
    time_diffs_airplate.append(t_avg)
    freq_airplate.append(1.0/t_avg)



altitude_drone = np.array(altitude_drone)/10
t_drone = np.linspace(0,t_end_drone-t_start_drone,num=len(freq_drone))
t_airplate = np.linspace(0,t_end_airplate-t_start_airplate,num=len(freq_airplate))
fig, ax1 = plt.subplots()
color = 'tab:blue'
ax1.set_xlabel("Time [s]")
ax1.set_ylabel("Frequency [Hz]",color=color)
ax1.plot(t_drone,freq_drone,color=color)

color='tab:orange'
ax2 = ax1.twinx()
ax2.set_ylabel("Altitude [m]",color=color)
ax2.plot(t_drone,altitude_drone-min(altitude_drone),color=color)
plt.grid()
plt.title("Moving average of message frequency, windowsize = "+str(window_size))
plt.savefig("../figures/drone_frequency.png")
plt.show()


fig, ax1 = plt.subplots()
color = 'tab:blue'
ax1.set_xlabel("Time [s]")
ax1.set_ylabel("Frequency [Hz]",color=color)
ax1.plot(t_airplate,freq_airplate,color=color)
plt.grid()
plt.title("Moving average of message frequency, windowsize = "+str(window_size))
plt.savefig("../figures/airplate_frequency.png")
plt.show()

# plt.xlabel("Time [s]")
# # plt.ylabel("Frequency [Hz]")
# plt.plot(x,altitude-min(altitude))
# plt.grid()
# plt.legend(["Average frequency [Hz]", "Flying altitude [m]"])
# plt.show()




# for prev, next in sliding_window(packets, 2):
    # msg_count_prev = prev["Dot11EltVendorSpecific"].info[4]
    # msg_count_next = next["Dot11EltVendorSpecific"].info[4]
    # time_diff = float(next.time - prev.time)
    # if msg_count_prev != msg_count_next and time_diff<10:
        # time_diffs.append(time_diff)
        # freq.append(1/time_diff)

# m = np.mean(time_diffs)

# print(" mean period, mean frequency:",m,1/m)
# plt.plot(freq)
# plt.grid()
# plt.show()
# plt.hist(time_diffs,bins=15)
# plt.show()
# while(reader.next()):
#     p = reader.next()
#     id = st = str(p["Dot11EltVendorSpecific"].info[10:30],"utf-8").strip('\x00')
#     if(id in ID_list):
#         index = ID_list.index(id)
#         count_list[index] += 1
        
#     print(count_list)
    


    
    


# plt.hist(list(filter(lambda latency: latency < 0.0035, time_diffs)), bins=100, color='blue', edgecolor='black')

# plt.title('Histogram of Values')
# plt.xlabel('Latency [s]')
# plt.ylabel('Frequency')

# plt.show()

# input()
