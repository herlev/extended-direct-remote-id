import pandas as pd
import statistics
import json

# https://stackoverflow.com/questions/7001144/range-over-characters-in-python
def char_range(c1, c2):
    for c in range(ord(c1), ord(c2)+1):
        yield chr(c)

receiver_locations = {}

for receiver in char_range('A', 'M'):
  data = pd.read_csv(f"../data/locations/RX_{receiver}/Raw Data.csv")
  lat = statistics.median(data["Latitude (°)"])
  lon = statistics.median(data["Longitude (°)"])
  receiver_locations[receiver] = (lat, lon)

json.dump(receiver_locations, open("receiver_locations.json", 'w'), indent=2)
