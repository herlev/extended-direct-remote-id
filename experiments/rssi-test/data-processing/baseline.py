import numpy as np
import matplotlib.pyplot as plt
names = ["nwb","neb","swb","seb"]
baselines = []
print("Individual RSSI mean")
for i in range(4):  
    fname = "../data/"+names[i]+".txt"
    data = np.loadtxt(fname,delimiter="\t",skiprows=7,dtype=int)
    mean = (np.mean(data[:,0]))
    baselines.append(mean)
    median = np.median(data[:,0])
    print(names[i],np.round(mean,3),median)
print("Overall mean RSSI")
print(np.mean(np.round(baselines,3)))
