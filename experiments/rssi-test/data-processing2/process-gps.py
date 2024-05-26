import pandas as pd
import statistics
import json

# https://stackoverflow.com/questions/7001144/range-over-characters-in-python
def char_range(c1, c2):
    for c in range(ord(c1), ord(c2)+1):
        yield chr(c)

receiver_locations = {}

for receiver in char_range('A', 'R'):
  data = pd.read_csv(f"../data-experiment2/Locations/RX_{receiver}/Raw Data.csv")
  lat = statistics.median(data["Latitude (°)"])
  lon = statistics.median(data["Longitude (°)"])
  receiver_locations[receiver] = (lat, lon)

test_locations = {}

for test_num in range(15):
  data = pd.read_csv(f"../data-experiment2/Locations/TX_{test_num+1}/Raw Data.csv")
  lat = statistics.median(data["Latitude (°)"])
  lon = statistics.median(data["Longitude (°)"])
  test_locations[test_num] = (lat, lon)

json.dump(test_locations, open("test_locations.json", 'w'), indent=2)
json.dump(receiver_locations, open("receiver_locations.json", 'w'), indent=2)
