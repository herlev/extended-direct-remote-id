#!/usr/bin/env python3
import matplotlib.pyplot as plt
import numpy as np
import seaborn as sns
import json
import math


clrs = sns.color_palette("husl", 5)

def close_circle(a):
  a.append(a[0])


for antenna_type in ["Whip", "FPC"]:
  readings = json.load(open(f"../data/{antenna_type}.json", "r"))
  max_rssi = max([reading['Mean'] for reading in readings])
  std = [reading['Std'] for reading in readings]

  # Data
  theta = [reading['Angle']/180 * math.pi for reading in readings]
  r = [reading['Mean'] - max_rssi for reading in readings]
  close_circle(std)
  close_circle(theta)
  close_circle(r)
  r = np.array(r)

  # Create polar plot
  plt.figure(figsize=(6,6))
  ax = plt.subplot(111, polar=True)  # Create polar subplot

  # Plot data
  ax.plot(theta, r)
  ax.fill_between(theta, r-std, r+std, alpha=0.4, facecolor=clrs[0])
  # ax.errorbar(theta, r, yerr=std, fmt='-o', label='Data with Error Bars')


  label_position=ax.get_rlabel_position()
  ax.text(np.radians(label_position+10), -5,'RSSI [dBm]',
          rotation=label_position,ha='center',va='center')

  # Customize the plot
  ax.set_title("H-Plane: 2.4GHz channel 6")
  ax.set_xlabel("Angle [°]")
  ax.set_ylim([-10, 1])

  plt.savefig(f"../figures/radiation-pattern-{antenna_type}.png", bbox_inches='tight')
  # Show the plot
  plt.show()
