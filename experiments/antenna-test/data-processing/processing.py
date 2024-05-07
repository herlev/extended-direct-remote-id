import numpy as np
import matplotlib.pyplot as plt
import json
num_skip = 13

# For the FPC antenna:
# folder_normal = "../data/Flat/Normalmount/flat"
# folder_rotated = "../data/Flat/Rotatedmount/flat"
# output_file = "../data/FPC.json"

# For the whip antenna:
folder_normal = "../data/Pointy/Normalmount/pointy"
folder_rotated = "../data/Pointy/Rotatedmount/pointy"
output_file = "../data/Whip.json"


folders = [folder_normal,folder_rotated]
substring = "Angle: "
rssi_list = []
mean_list = []
ml = np.zeros((72,5))
sl = np.zeros((72,5))
std_list = []
for k in range(len(folders)):
    for l in range(5):
        file = open(folders[k]+str(l)+".txt")
        lines = file.readlines()
        for i in range(num_skip,len(lines)):
            if(substring in lines[i]):
                if (lines[i] == "Angle: 180\n"):
                    continue
                for j in range(i+1,i+201):
                    data = lines[j].split("\t")
                    rssi_list.append(int(data[0]))
                mean_list.append(np.mean(rssi_list))
                std_list.append(np.std(rssi_list))
                rssi_list = []
        if(k == 1):
            mean_list.reverse()
            std_list.reverse()
        ml[36*k:36*(k+1),l] = mean_list
        sl[36*k:36*(k+1),l] = std_list
        mean_list = []
        std_list = []

mean = np.mean(ml,axis=1)
std = np.mean(sl,axis=1)
dict_list = []
for i in range(len(mean)):
    data = {"Angle": i*5,
        "Mean": mean[i],
        "Std": std[i]}
    dict_list.append(data)
with open(output_file, "w") as json_file:
    json.dump(dict_list, json_file,indent=1)
