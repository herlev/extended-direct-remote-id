import numpy as np
import matplotlib.pyplot as plt
filename_wifi = "../data/wifidata.txt"
filename_bt = "../data/btdata.txt"
filename_bt_groundtruth = "../data/btdata_groundtruth.txt"
data_wifi = np.loadtxt(filename_wifi,delimiter="\t",skiprows=90,dtype=str)
data_bt = np.loadtxt(filename_bt,delimiter="\t",skiprows=62,dtype=str)
data_bt_groundtruth = np.loadtxt(filename_bt_groundtruth,delimiter="\t",skiprows=62,dtype=str)
bit_rates = ["1M_L","12M","24M","54M"]

distances = [58.90623594596779,
 109.15019712322665,
 160.1028091738484,
 212.56816475291467,
 261.619196397568,
 317.2779797111629,
 370.12695107095215,
 420.9482877358878,
 473.09968495932327,
 528.2772985858265,
 579.1008218232791,
 632.2606373655719,
 684.584781024206,
 737.7565075533936,
 789.624291517391,
 838.6650287554644,
 890.5318860030997,
 915.6956411122294,
 941.0007417230563,
 968.3018882109238,
 992.3531536970482]




max_block_length = 21
for j in range(len(bit_rates)):
    block_length = []
    rssi = []
    mask = (data_wifi[:,3] == bit_rates[j])
    data_masked = data_wifi[mask,:]
    threshold = 50*1000000  # 50 seconds

    block_starts = [0]
    block_ends = []
    for i in range(1, len(data_masked)):
        if abs(int(data_masked[i][1]) - int(data_masked[i - 1][1])) > threshold:
            block_ends.append(i - 1)
            block_starts.append(i)


    block_ends.append(len(data_masked) - 1)

    for start, end in zip(block_starts, block_ends):
        block_data = data_masked[start:end + 1]
        rssi_int = np.array(data_masked[start:end + 1,4],dtype=int)
        rssi.append(np.mean(rssi_int))
        block_length.append(block_data.shape[0]/500)
    block_length += [0] * (max_block_length - len(block_length))
    plt.plot(distances[0:len(block_length)],block_length)

# plt.savefig("../figures/Range-test-wifi.png")
# plt.show()




## BT ground truth processing
block_start_bt_gt = [0]
block_end_bt_gt = []
block_length_bt_gt = []
rssi_gt = []
for i in range(1, len(data_bt_groundtruth)):

    if abs(int(data_bt_groundtruth[i][1]) - int(data_bt_groundtruth[i - 1][1])) > threshold:
        block_end_bt_gt.append(i - 1)
        block_start_bt_gt.append(i)
block_end_bt_gt.append(len(data_bt_groundtruth)-1)
for start, end in zip(block_start_bt_gt, block_end_bt_gt):
    block_data = data_bt_groundtruth[start:end + 1]
    rssi_int = np.array(data_bt_groundtruth[start:end + 1,2],dtype=int)
    rssi_gt.append(np.mean(rssi_int))
    block_length_bt_gt.append(block_data.shape[0])




## BT processing
block_length_bt = []
block_starts_bt = [0]
block_ends_bt = []
rssi_bt = []
for i in range(1, len(data_bt)):
    if abs(int(data_bt[i][1]) - int(data_bt[i - 1][1])) > threshold:
        block_ends_bt.append(i - 1)
        block_starts_bt.append(i)

block_ends_bt.append(len(data_bt) - 1)
for start, end in zip(block_starts_bt, block_ends_bt):
    block_data = data_bt[start:end + 1]
    rssi_int = np.array(data_bt[start:end + 1,2],dtype=int)
    rssi_bt.append(np.mean(rssi_int))
    block_length_bt.append(block_data.shape[0])
    
# print(block_length_bt)
print(block_length_bt_gt)

plt.plot(distances,list(map(lambda t: t[0]/t[1],zip(block_length_bt,block_length_bt_gt))))
legend_list = ["Wi-Fi 1Mb/s","Wi-Fi 12Mb/s","Wi-Fi 24 Mb/s","Wi-Fi 54 Mb/s","Bluetooth 5"]
plt.legend(legend_list,fontsize=7,loc='upper right')
plt.grid(visible=True)
plt.ylabel("Receive rate %",fontsize=14)
plt.xlabel("Distance [m]",fontsize=14)
plt.savefig("../figures/Range-test.png")
plt.show()




