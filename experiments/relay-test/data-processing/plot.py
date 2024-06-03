import folium
import json
import pandas as pd
import statistics
import json

locations = {}

for receiver in ["00", "5c", "9c", "14", "64", "68", "84", "90", "a8", "e8", "f4"]:
  data = pd.read_csv(f"../data/{receiver}/Raw Data.csv")
  lat = statistics.median(data["Latitude (°)"])
  lon = statistics.median(data["Longitude (°)"])
  locations[receiver] = (lat, lon)


center_lat = sum(lat for lat, lon in locations.values()) / len(locations)
center_lon = sum(lon for lat, lon in locations.values()) / len(locations)
map = folium.Map(location=[center_lat, center_lon], zoom_start=14.5,control_scale=True)

for receiver, location in locations.items():
  folium.CircleMarker(location,radius=4,fill_opacity=1,opacity=1,fill=True, color="black").add_to(map)

map.save('map.html')
