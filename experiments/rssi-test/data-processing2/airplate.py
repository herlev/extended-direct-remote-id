import pyproj
import numpy as np
import folium
import json
import matplotlib.pyplot as plt
from scipy.optimize import minimize
import csv
# WCWCL source: https://www.jocm.us/uploadfile/2014/0325/20140325011006972.pdf
P = pyproj.Proj(proj='utm', zone=32, ellps='WGS84', preserve_units=True)
G = pyproj.Geod(ellps='WGS84')

def LatLon_To_XY(Lat, Lon):
    return np.array(P(Lon, Lat)) 

def XY_To_LonLat(x,y):
    return P(x, y, inverse=True)    

def sqrt_eq(RSSI,g=1): # Helper function for equation 8 in WCWCL
    return np.sqrt(np.power(np.power(10,RSSI/10),g))

def weights_from_RSSI(RSSI_list,g=1): # Equation 8 WCWCL
    weights = np.zeros((len(RSSI_list),1))
    divisor = sum((map(lambda rssi: sqrt_eq(rssi,g),RSSI_list)))
    for i in range(len(RSSI_list)):
        weights[i,0] =sqrt_eq(RSSI_list[i],g)/divisor
    return np.array(weights)


def normalized_weights(weights): # Equation 10 WCWCL
    n = len(weights)
    divisor = sum(w*pow(n,2*w) for w in weights[:,0])
    weights_normalized = np.zeros((n,1))
    for i in range(n):
        weights_normalized[i,0] = (weights[i,0]*np.power(n,2*weights[i,0]))/divisor
    return weights_normalized
    
def position_estimate(weights,positions): # Equation 11 WCWCL
    return np.sum(weights*positions,axis=0)

def moving_average(a, n=3):
    ret = np.cumsum(a, dtype=float)
    ret[n:] = ret[n:] - ret[:-n]
    return ret[n - 1:] / n

def main():

    f = open("receiver_locations.json","r")
    rx_locations = json.load(f)
    del rx_locations['E'] # Remove entry E due to destroyed batteryconnectors on this receiver
    A = rx_locations["A"]
    A_UTM =LatLon_To_XY(A[0],A[1])
    UTM_list = []
    for key,value in rx_locations.items():
        UTM_list.append(LatLon_To_XY(value[0],value[1])-A_UTM)
    center_lat = sum(lat for lat, lon in rx_locations.values()) / len(rx_locations)
    center_lon = sum(lon for lat, lon in rx_locations.values()) / len(rx_locations)
    map = folium.Map(location=[center_lat+0.0003, center_lon+0.002], zoom_start=18,control_scale=True)

    pos_list_UTM = []
    rssi = np.loadtxt(f"points/{15}.txt",delimiter=",",skiprows=0,dtype=int)
    for i in range(len(rssi)):
        RSSI_list = rssi[i,1:]
        
        # WCWCL localization
        w = weights_from_RSSI(RSSI_list,1.8)
        w_normalized = normalized_weights(w)
        p_norm = position_estimate(w_normalized,UTM_list)
        pos_list_UTM.append(p_norm)

    for receiver, location in rx_locations.items():
        folium.CircleMarker(location,radius=5,fill_opacity=1,fill=True, color="black").add_to(map)
    
    # Add true transmitter llocation to map    

    # Read Airplate GPS data
    file = "../data-experiment2/Locations/airplateGPS.csv"
    skip_rows = 30
    lat_list = []
    lon_list = []
    with open(file, 'r') as file:
        reader = csv.reader(file)
        
        # Skip the initial rows
        for _ in range(skip_rows):
            next(reader)
        
        # Loop through the remaining rows in the CSV
        for row in reader:
            # Extract the first two items
            lat = row[0]
            lon = row[1]

            lat_list.append(lat)
            lon_list.append(lon)
    
    map.save('map.html')
    # Convert Airplate lat,lon to UTM and offset with receiver A's location
    x_airplate = []
    y_airplate = []
    tx_coordinates = []
    for i in range(len(lat_list)):
        lat = lat_list[i]
        lon = lon_list[i]
        latlon = [float(lat),float(lon)]
        tx_coordinates.append(latlon)
        p = LatLon_To_XY(lat,lon)-A_UTM
        x_airplate.append(p[0])
        y_airplate.append(p[1])
    
    
    # Perform moving average on the estimated positions
    window_size = 200
    # x = moving_average(np.array(pos_list_UTM)[:,0],window_size)
    # y = moving_average(np.array(pos_list_UTM)[:,1],window_size)
    x = np.array(pos_list_UTM)[:,0]
    y = np.array(pos_list_UTM)[:,1]
    tx_lonlat = []
    for i in range(len(x)):
        p = [x[i],y[i]]+A_UTM
        p_lonlat = XY_To_LonLat(p[0],p[1])
        tx_lonlat.append([p_lonlat[1],p_lonlat[0]])
    print(tx_lonlat)
    folium.PolyLine(tx_lonlat,smoothFactor=0, color="blue").add_to(map)
    folium.PolyLine(tx_coordinates,smoothFactor=0, color="red").add_to(map)
    # # plt.plot(x[0],y[0],'bo',label="Filtered WCWCL location estimate")
    # for i in range(0, len(x), 1):
    #     if(i==1):
    #         plt.plot(x[i:i+2], y[i:i+2], 'b-',label="Filtered WCWCL location estimate")
    #     else:
    #         plt.plot(x[i:i+2], y[i:i+2], 'b-')

    

    
    # # plt.plot(x_airplate[0],y_airplate[0],'ko')
    # for i in range(0, len(x_airplate), 1):
    #     if(i==1):
    #         plt.plot(x_airplate[i:i+2], y_airplate[i:i+2], 'k-',label="Location from AirPlate GPS")
    #     else:
    #         plt.plot(x_airplate[i:i+2], y_airplate[i:i+2], 'k-')
    # plt.plot(np.array(UTM_list)[:,0],np.array(UTM_list)[:,1],'go',label="Receiver locations")
    # plt.grid()
    # plt.axis('equal')
    # plt.axis([-20, 160, -90,90])
    # plt.xlim([-20,160])
    # plt.xlabel("x [m]")
    # plt.ylabel("y [m]")
    # plt.ylim([-90,90])
    # plt.legend()
    # plt.savefig("../figures/airplateflight.png")
    # plt.show()

    map.save('map.html')



    
if __name__ == "__main__":
    main()