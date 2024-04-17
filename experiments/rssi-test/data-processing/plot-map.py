import folium
import json

locations = json.load(open("receiver_locations.json", "r"))
center_lat = sum(lat for lat, lon in locations.values()) / len(locations)
center_lon = sum(lon for lat, lon in locations.values()) / len(locations)
map = folium.Map(location=[center_lat, center_lon], zoom_start=10)

for receiver, location in locations.items():
  folium.Marker(location, icon=folium.Icon(color="black")).add_to(map)

map.save('map.html')
