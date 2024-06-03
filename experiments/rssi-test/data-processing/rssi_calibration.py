import os
import glob
import numpy as np
import json
# Define the folder path
folder_path = '../data/rssi_calibration'

# Use glob to find all text files in the folder
txt_files = glob.glob(os.path.join(folder_path, '*.txt'))

# Read and print the content of each text file
dict_list = []
for file_path in txt_files:
    rssi_data = np.loadtxt(file_path,delimiter="\t",skiprows=3,dtype=int)
    name = file_path.replace(folder_path+"/rssi-","")
    name = name.replace(".txt","")
    if(name[0:3] == "fpc"):
        distance = name[4:7]
    else:
        distance = name[5:8]
    data = {"Antenna": name,
        "Distance": int(distance),
        "Mean": np.mean(rssi_data),
        "Std": np.std(rssi_data)}
    dict_list.append(data)

output_file = "../data/rssi_calibration.json"
with open(output_file, "w") as json_file:
    json.dump(dict_list, json_file,indent=1)
    
    
