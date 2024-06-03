import pyproj
import numpy as np
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

def normalized_weights(weights):
    n = len(weights)
    divisor = sum(w*pow(n,2*w) for w in weights[:,0])
    weights_normalized = np.zeros((n,1))
    for i in range(n):
        weights_normalized[i,0] = (weights[i,0]*np.power(n,2*weights[i,0]))/divisor
    return weights_normalized
    
def position_estimate(weights,positions):
    return np.sum(weights*positions,axis=0)

def mean_norm(true_point,point_list):
        e = np.mean([np.linalg.norm(true_point-np.array(coord)) for coord in point_list])
        return np.mean(e)
def mean_squared_error(true_point, point_list):
        squared_error = [(true_point-np.array(coord))**2 for coord in point_list]
        mse = np.mean([np.sum(sq_err) for sq_err in squared_error])
        return mse

def main():
    f = open("receiver_locations.json","r")
    locations = json.load(f)
    A = locations["A"]
    A_UTM =LatLon_To_XY(A[0],A[1])
    UTM_list = []
    for key,value in locations.items():
        UTM_list.append(LatLon_To_XY(value[0],value[1])-A_UTM)   

    f_tx = open("test_locations.json","r")
    locations = json.load(f_tx)
    tx_points = []
    for receiver, location in locations.items():
        tx_points.append(LatLon_To_XY(location[0],location[1])-A_UTM)

    norm_list = []
    mse_list = []
    g_list = np.linspace(0,5,50)
    for g in range(len(g_list)):
        g = g_list[g]
        receiver_list = []
        for j in range(len(tx_points)):
            point_list = []
            rssi = np.loadtxt(f"points/{j}.txt",delimiter=",",skiprows=0,dtype=int)
            for i in range(len(rssi)):
                RSSI_list = rssi[i,1:]
                w = weights_from_RSSI(RSSI_list,g)
                w_normalized = normalized_weights(w)
                p_norm = position_estimate(w_normalized,UTM_list)
                point_list.append(p_norm)
            receiver_list.append(point_list)
        norm_sum = 0
        mse_sum = 0
        for i in range(len(receiver_list)):
            true_point = np.array(tx_points[i])
            norm = mean_norm(true_point,receiver_list[i])
            norm_sum = norm_sum+norm
            # mse = mean_squared_error(true_point,receiver_list[i])
            # mse_sum = mse_sum + mse

        norm_list.append(norm_sum/8)
        mse_list.append(mse_sum/8)
            
    plt.plot(g_list,norm_list,label="Mean euclidean distance")
    plt.xlabel("g")
    plt.ylabel("Error [m]")
    plt.grid()
    plt.legend()
    plt.savefig("../figures/norm_error_experiment1.png")
    plt.show()



    
if __name__ == "__main__":
    main()