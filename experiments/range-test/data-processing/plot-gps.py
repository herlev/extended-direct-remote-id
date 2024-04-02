import folium
import pandas as pd
import random
import statistics
from geopy.distance import great_circle
from geopy.distance import geodesic

def generate_random_hex_color():
    r = random.randint(0, 255)
    g = random.randint(0, 255)
    b = random.randint(0, 255)
    hex_color = '#{0:02x}{1:02x}{2:02x}'.format(r, g, b)
    return hex_color

data = pd.read_csv("../data/Location_GPS_2024-03-15_12-36-36/Raw Data.csv")
coordinates = list(zip(data["Time (s)"], data["Latitude (°)"], data["Longitude (°)"]))

time_data = pd.read_csv("../data/Location_GPS_2024-03-15_12-36-36/meta/time.csv")
pause_times = time_data[time_data["event"] == "PAUSE"]["experiment time"]

gps_groups = dict(map(lambda t: (t, []), pause_times))

def get_time(t):
    for pause_time in pause_times:
        if t < pause_time:
            return pause_time
    raise "no"

for time, lat, lon in coordinates:
    gps_groups[get_time(time)].append((lat,lon))


center_lat = sum(lat for _, lat, _ in coordinates) / len(coordinates)
center_lon = sum(lon for _, _, lon in coordinates) / len(coordinates)
map = folium.Map(location=[center_lat, center_lon], zoom_start=10)

colors = [
    'red',
    'blue',
    'gray',
    'darkred',
    'lightred',
    'orange',
    'beige',
    'green',
    'darkgreen',
    'lightgreen',
    'darkblue',
    'lightblue',
    'purple',
    'darkpurple',
    'pink',
    'cadetblue',
    'lightgray',
    # 'black'
]

mid_points = []

for i, group in enumerate(gps_groups.values()):
    unzip = list(zip(*group))
    mid_lat = statistics.median(unzip[0])
    mid_lon = statistics.median(unzip[1])
    mid_points.append((mid_lat, mid_lon))

    color = colors[i % len(colors)]
    # for coord in group:
        # folium.Marker(coord, icon=folium.Icon(color=color)).add_to(map)
    
start_point = (55.3650529, 10.4308716)
start_point = (55.365035,10.431033)

for coord in mid_points:
    folium.Marker(coord, icon=folium.Icon(color="black")).add_to(map)

folium.Marker(start_point, icon=folium.Icon(color="red")).add_to(map)

dists = []

for p in mid_points:
    dists.append(geodesic(start_point, p).meters)

map.save('../figures/coordinates_on_osm.html')
# print(great_circle(start_point, mid_points[-1]))
# print(geodesic(start_point, mid_points[-1]))

