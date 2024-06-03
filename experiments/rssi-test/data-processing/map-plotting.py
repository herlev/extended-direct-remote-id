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

def distance_estimate(rssi_list, db0,pathloss,reference_distance=1):
    dist_list = []
    for i in range(len(rssi_list)):
        d = reference_distance*pow(10,(db0-rssi_list[i])/(10*pathloss))
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
    
    # Read mean RSSI value db0 at distance d0 from calibration data
    f=open("../data/rssi_calibration.json","r") 
    rssi_calibration = json.load(f)
    db0 = rssi_calibration[6]["Mean"]
    db0_dist = rssi_calibration[6]["Distance"]/100
    
    
    f = open("receiver_locations.json","r")
    rx_locations = json.load(f)
    A = rx_locations["A"]
    A_UTM =LatLon_To_XY(A[0],A[1])
    UTM_list = []
    for key,value in rx_locations.items():
        UTM_list.append(LatLon_To_XY(value[0],value[1])-A_UTM)
    center_lat = sum(lat for lat, lon in rx_locations.values()) / len(rx_locations)
    center_lon = sum(lon for lat, lon in rx_locations.values()) / len(rx_locations)
    map = folium.Map(location=[center_lat+0.0002, center_lon+0.001], zoom_start=18,control_scale=True)

    j = 7 # The specific transmitter location we want to show on the map
    pos_list_WCWCL = []
    pos_list_multilat = []
    rssi = np.loadtxt(f"points/{j}.txt",delimiter=",",skiprows=0,dtype=int)
    for i in range(len(rssi)):
        RSSI_list = rssi[i,1:]
        
        # WCWCL localization
        w = weights_from_RSSI(RSSI_list,1.5)
        w_normalized = normalized_weights(w)
        p_norm = position_estimate(w_normalized,UTM_list)
        point_norm = p_norm+A_UTM
        latlon_norm = XY_To_LonLat(point_norm[0],point_norm[1])
        pos_list_WCWCL.append(latlon_norm)
        
        # Multilateration
        dist_list = distance_estimate(RSSI_list,db0,2,db0_dist)
        p_multilat_UTM = multilateration(dist_list,UTM_list)+A_UTM
        p_multilat_latlon = XY_To_LonLat(p_multilat_UTM[0],p_multilat_UTM[1])
        pos_list_multilat.append(p_multilat_latlon)
        
        folium.CircleMarker([p_multilat_latlon[1],p_multilat_latlon[0]],radius=1,fill_opacity=0.5,opacity=0.5,fill=False,stroke=True, color="green").add_to(map)
        folium.CircleMarker([latlon_norm[1],latlon_norm[0]],radius=1,fill_opacity=0.5,opacity=0.5,fill=False,stroke=True, color="blue").add_to(map)
    
    # Calculate and plot mean of both location estimates
    mean_multilat = np.mean(np.array(pos_list_multilat),axis=0)
    mean_WCWCL = np.mean(np.array(pos_list_WCWCL),axis=0)
    folium.CircleMarker([mean_WCWCL[1],mean_WCWCL[0]],radius=5,fill_opacity=1,fill=True, color="darkblue").add_to(map)
    folium.CircleMarker([mean_multilat[1],mean_multilat[0]],radius=5,fill_opacity=1,fill=True, color="darkgreen").add_to(map)
  
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