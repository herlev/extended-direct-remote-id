import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt
from matplotlib.ticker import MultipleLocator


def read_file(filename):
  f = open(filename, 'r')
  return [float(line.strip()) for line in f]


files = ["latency-test-YS7-10min-latencies", "latency-test-usb-adapter-YS7-10min-latencies"]
titles = ["Latency measured on Lenovo Yoga Slim 7 (Intel Wi-Fi 6 AX200)", "Latency measured on Lenovo Yoga Slim 7 (TP-Link Archer T3U Plus)"]

for i, file in enumerate(files):
  sns.set(style="ticks")
  x = read_file(f"../data/{file}.txt")
  x = list(map(lambda n: n*1000, x)) # convert to ms

  f, (ax_box, ax_hist) = plt.subplots(2, sharex=True, gridspec_kw={"height_ratios": (.40, .60)})

  sns.boxplot(x=x, ax=ax_box, medianprops=dict(color='orange'))
  sns.histplot(x=x, bins=400, stat='percent', ax=ax_hist)
  plt.gca().xaxis.set_major_locator(MultipleLocator(1))
  plt.xlabel("Latency [ms]")
  plt.suptitle(titles[i])


  ax_box.set(yticks=[])
  sns.despine(ax=ax_hist)
  sns.despine(ax=ax_box, left=True)

  aspect_ratio = 4/1
  fig = plt.gcf()
  s = 10
  fig.set_size_inches(s, s / aspect_ratio)  # Adjust the width and height based on the aspect ratio
  plt.savefig(f"../figures/{file}.svg", bbox_inches='tight')
