import itertools
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt

# https://stackoverflow.com/questions/7001144/range-over-characters-in-python
def char_range(c1, c2):
    for c in range(ord(c1), ord(c2)+1):
        yield chr(c)

# https://napsterinblue.github.io/notes/python/internals/itertools_sliding_window/
def sliding_window(iterable, n=2):
    iterables = itertools.tee(iterable, n)
    for iterable, num_skipped in zip(iterables, itertools.count()):
        for _ in range(num_skipped):
            next(iterable, None)
    return zip(*iterables)

def parse_line(line):
  return list(map(lambda s: int(s), line.strip().split('\t')))

def parse_file(file_name):
  f = open(file_name)
  lines = f.readlines()[7:]
  return [parse_line(line) for line in lines]

def group_data(data):
  groups = [[data[0]]]

  for prev, next in sliding_window(data):
    if next[1] < prev[1]:
      groups.append([])
    groups[-1].append(next)

  return groups

receiver_readings = {}
for receiver in char_range('A', 'M'):
  data = np.loadtxt(f"../data/manual/{receiver}.txt",delimiter="\t",skiprows=7,usecols=(0,1),dtype=int)
  receiver_readings[receiver] = data


receiver_readings = {receiver: group_data(data) for receiver, data in receiver_readings.items() }

for i in range(len(receiver_readings['A'])):
  f = open(f"points/{i}.txt", "w")
  common_timestamps = set(np.array(receiver_readings['A'][i])[:,1])
  for receiver in char_range('B', 'M'):
    common_timestamps = common_timestamps & set(np.array(receiver_readings[receiver][i])[:,1])

  common_receiver_readings = {}
  for receiver in char_range('A', 'M'):
    common_receiver_readings[receiver] = list(filter(lambda e: e[1] in common_timestamps, receiver_readings[receiver][i]))

  for i in range(len(common_receiver_readings['A'])):
    timestamp = common_receiver_readings['A'][i][1]
    f.write(f"{timestamp}")
    for receiver in char_range('A', 'M'):
      rssi = common_receiver_readings[receiver][i][0]
      f.write(f",{rssi}")
    f.write("\n")
