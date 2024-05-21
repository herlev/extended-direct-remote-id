import pyproj
import numpy as np
import folium
import json
import matplotlib.pyplot as plt
P = pyproj.Proj(proj='utm', zone=32, ellps='WGS84', preserve_units=True)
G = pyproj.Geod(ellps='WGS84')

def LatLon_To_XY(Lat, Lon):
    return np.array(P(Lon, Lat)) 

def XY_To_LonLat(x,y):
    return P(x, y, inverse=True)    

def sqrt_eq(RSSI,g=1):
    return np.sqrt(np.power(np.power(10,RSSI/10),g))

def weights_from_RSSI(RSSI_list,g=1):
    weights = np.zeros((len(RSSI_list),1))
    divisor = sum((map(lambda rssi: sqrt_eq(rssi,g),RSSI_list)))
    for i in range(len(RSSI_list)):
        weights[i,0] =sqrt_eq(RSSI_list[i],g)/divisor
    return np.array(weights)

colors = [
    # 'red',
    'blue',
    'gray',
    # 'darkred',
    # 'lightred',
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
    'lightgray'
]
def normalized_weights(weights):
    n = len(weights)
    divisor = sum(w*pow(n,2*w) for w in weights[:,0])
    weights_normalized = np.zeros((n,1))
    for i in range(n):
        weights_normalized[i,0] = (weights[i,0]*np.power(n,2*weights[i,0]))/divisor
    return weights_normalized
    
def position_estimate(weights,positions):
    return np.sum(weights*positions,axis=0)

def main():
    f = open("receiver_locations.json","r")
    locations = json.load(f)
    A = locations["A"]
    A_UTM =LatLon_To_XY(A[0],A[1])
    UTM_list = []
    for key,value in locations.items():
        UTM_list.append(LatLon_To_XY(value[0],value[1])-A_UTM)
    # UTM_list = UTM_list[0:-3]    
    center_lat = sum(lat for lat, lon in locations.values()) / len(locations)
    center_lon = sum(lon for lat, lon in locations.values()) / len(locations)
    map = folium.Map(location=[center_lat, center_lon], zoom_start=18)
    for receiver, location in locations.items():
        folium.Marker(location, icon=folium.Icon(color="black")).add_to(map)
    
    receiver_list = []
    # for j in range(1):
    point_list = []
    j = 1
    rssi = np.loadtxt(f"points/{j}.txt",delimiter=",",skiprows=0,dtype=int)
    for i in range(len(rssi)):
        RSSI_list = rssi[i,1:]
        w = weights_from_RSSI(RSSI_list,1)
        # p = position_estimate(w,UTM_list)
        # latlon = XY_To_LonLat(point[0],point[1])
        # point = p+A_UTM
        w_normalized = normalized_weights(w)
        # print(w_normalized)
        # w_normalized[5:8] = 0
        p_norm = position_estimate(w_normalized,UTM_list)
        # print("Novel estimate",p_norm)
        point_norm = p_norm+A_UTM
        point_list.append(p_norm)
        latlon_norm = XY_To_LonLat(point_norm[0],point_norm[1])
        folium.Marker([latlon_norm[1],latlon_norm[0]], icon=folium.Icon(color=colors[j])).add_to(map)
    receiver_list.append(point_list)
    f = open("test_locations.json","r")
    locations = json.load(f)
    center_lat = sum(lat for lat, lon in locations.values()) / len(locations)
    center_lon = sum(lon for lat, lon in locations.values()) / len(locations)
    tx_points = []
    location = locations[str(j)]
    tx_points.append(LatLon_To_XY(location[0],location[1])-A_UTM)
    folium.Marker(location, icon=folium.Icon(color="red"),popup=f"{receiver}").add_to(map)
    # for receiver, location in locations.items():
    #     tx_points.append(LatLon_To_XY(location[0],location[1])-A_UTM)
    #     folium.Marker(location, icon=folium.Icon(color="red"),popup=f"{receiver}").add_to(map)

    map.save('map.html')
    
    # colors2 = ["bo","go","co","mo","yo","b^","g^","c^","m^"]
    # UTM_list = np.array(UTM_list)
    # for i in range(len(receiver_list)):
    #     points = np.array(receiver_list[i])
    #     plt.plot(np.mean(points[:,0]),np.mean(points[:,1]),colors2[i])
    # plt.plot(np.array(tx_points)[:,0],np.array(tx_points)[:,1],'ko')
    # plt.plot(UTM_list[:,0],UTM_list[:,1],"ro")
    # # plt.axis("equal")
    # plt.xlim([-20,200])
    # plt.ylim([-20,200])
    # plt.show()


    
if __name__ == "__main__":
    main()