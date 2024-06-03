import pyproj
import numpy as np
import json
from tqdm import tqdm
from scipy.optimize import minimize
import matplotlib.pyplot as plt
P = pyproj.Proj(proj='utm', zone=32, ellps='WGS84', preserve_units=True)
G = pyproj.Geod(ellps='WGS84')

def LatLon_To_XY(Lat, Lon):
    return np.array(P(Lon, Lat)) 

def XY_To_LonLat(x,y):
    return P(x, y, inverse=True)    

def distance_estimate(rssi_list, db0,pathloss,reference_distance=1):
    dist_list = []
    for i in range(len(rssi_list)):
        d = reference_distance*pow(10,(db0-rssi_list[i])/(10*pathloss))
        dist_list.append(d)
    return dist_list

def mean_norm(true_point,point_list):
        e = np.mean([np.linalg.norm(true_point-np.array(coord)) for coord in point_list])
        return np.mean(e)
def multilateration(dist_list, node_locations):#https://github.com/glucee/Multilateration
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

def pos_error(utm_pos,pos_estimate):
    return 
def main():
    
    # Read mean RSSI value db0 at distance d0 from calibration data
    f=open("../data/rssi_calibration.json","r") 
    rssi_calibration = json.load(f)
    db0 = rssi_calibration[6]["Mean"]
    db0_dist = rssi_calibration[6]["Distance"]/100
    
    f = open("receiver_locations.json","r")
    rx_locations = json.load(f)

    # Calculate UTM coordinates of all receivers
    A = rx_locations["A"]
    A_UTM =LatLon_To_XY(A[0],A[1])
    UTM_list = []
    for key,value in rx_locations.items():
        UTM_list.append(LatLon_To_XY(value[0],value[1])-A_UTM) 
        
    f = open("test_locations.json","r")
    tx_locations = json.load(f)
    # Calculate UTM coordinates of transmitter locations and add transmitter locations to map
    tx_points = []
    for receiver, location in tx_locations.items():
        tx_points.append(LatLon_To_XY(location[0],location[1])-A_UTM)

    norm_list = []
    pathloss_list = np.linspace(2,6,20)
    for p in tqdm(range(len(pathloss_list))):
        pathloss = pathloss_list[p]
        pos_est_list = []
        all_points_est = []
        for j in range(len(tx_points)):
            print(j)
            rssi = np.loadtxt(f"points/{j}.txt",delimiter=",",skiprows=0,dtype=int)
            pos_est_list = []
            for i in range(len(rssi)):
                RSSI_list = rssi[i,1:]
                distance_list = distance_estimate(RSSI_list,db0,pathloss=pathloss,reference_distance=db0_dist)
                pos_est = multilateration(distance_list,UTM_list)
                pos_est_list.append(pos_est)
            all_points_est.append(pos_est_list)
        norm_sum = 0
        for i in range(len(all_points_est)):
            true_point = np.array(tx_points[i])
            norm = mean_norm(true_point,all_points_est[i])
            norm_sum = norm_sum+norm
        norm_list.append(norm_sum/len(tx_points))

            
    plt.plot(pathloss_list,norm_list,label="Mean euclidean distance")
    plt.xlabel("Pathloss exponent")
    plt.ylabel("Error [m]")
    plt.grid()
    plt.legend()
    plt.savefig("../figures/multilateration_error_experiment1.png")
    plt.show()


    
if __name__ == "__main__":
    main()