import matplotlib.pyplot as plt
import numpy as np

probability = 0.95
max_nodes = 100
dist = 0.600
latency = 2.5
y = np.power(probability,np.linspace(1,max_nodes,max_nodes))*100
x = np.linspace(1,max_nodes,max_nodes)*dist
latency_x = np.linspace(1,max_nodes,max_nodes)*latency
fs = 13
plt.plot(x,y)
plt.grid()
plt.xlabel("Distance [km]",fontsize=fs)
plt.ylim([0,100])
plt.ylabel("Probability [%]",fontsize=fs)
plt.savefig("../figures/probability.png")
plt.show()

plt.plot(x,latency_x)
plt.grid()
plt.xlabel("Distance [km]",fontsize=fs)
plt.ylabel("Latency [ms]",fontsize=fs)
plt.savefig("../figures/probability_latency.png")
plt.show()