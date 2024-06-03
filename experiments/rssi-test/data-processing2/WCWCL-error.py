import pyproj
import numpy as np
import folium
import json
import matplotlib.pyplot as plt
from scipy.optimize import minimize
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

def distance_estimate(rssi_list, db0,pathloss):
    dist_list = []
    for i in range(len(rssi_list)):
        d = pow(10,(db0-rssi_list[i])/(10*pathloss))
        dist_list.append(d)
    return dist_list
def multilateration(dist_list, node_locations): #https://github.com/glucee/Multilateration
	def error(x, c, r):
		return sum([(np.linalg.norm(x - c[i]) - r[i]) ** 2 for i in range(len(c))])

	l = len(node_locations)
	S = sum(dist_list)
	# compute weight vector for initial guess
	W = [((l - 1) * S) / (S - w) for w in dist_list]
	# get initial guess of point location
	x0 = sum([W[i] * node_locations[i] for i in range(l)])
	# optimize distance from signal origin to border of spheres
	return minimize(error, x0, args=(node_locations, dist_list), method='Nelder-Mead').x

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

    j = 0 # The specific transmitter location we wnat to show on the map
    rssi = np.loadtxt(f"points/{j}.txt",delimiter=",",skiprows=0,dtype=int)
    for i in range(len(rssi)):
        RSSI_list = rssi[i,1:]
        
        # WCWCL localization
        w = weights_from_RSSI(RSSI_list,1.5)
        w_normalized = normalized_weights(w)
        p_norm = position_estimate(w_normalized,UTM_list)
        point_norm = p_norm+A_UTM
        latlon_norm = XY_To_LonLat(point_norm[0],point_norm[1])
        folium.CircleMarker([latlon_norm[1],latlon_norm[0]],radius=1,fill_opacity=1,opacity=0.5,fill=True, color="darkblue").add_to(map)
        
        # Multilateration
        dist_list = distance_estimate(RSSI_list,-25,2)
        p_multilat_UTM = multilateration(dist_list,UTM_list)+A_UTM
        p_multilat_latlon = XY_To_LonLat(p_multilat_UTM[0],p_multilat_UTM[1])
        folium.CircleMarker([p_multilat_latlon[1],p_multilat_latlon[0]],radius=1,fill_opacity=1,opacity=0.5,fill=True, color="green").add_to(map)
    
    # Add receivers to map
    for receiver, location in rx_locations.items():
        folium.CircleMarker(location,radius=5,fill_opacity=1,fill=True, color="black").add_to(map)
    
    # Add true transmitter llocation to map    
    f = open("test_locations.json","r")
    tx_locations = json.load(f)
    folium.CircleMarker(tx_locations[str(j)],radius=5,fill_opacity=1,fill=True, color="red").add_to(map)
  
    map.save('map.html')



    
if __name__ == "__main__":
    main()