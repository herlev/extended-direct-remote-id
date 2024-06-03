import pandas as pd
import csv
import numpy as np
from scapy.all import PcapNgReader

file = "../data-experiment2/Locations/airplatewalkcsv.csv"
skip_rows = 307
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
        first_item = row[0]
        second_item = row[1]
        
        # Convert to integers
        lon = int(first_item.strip('"'))/10e6
        lat = int(second_item.strip('"'))/10e6
        lat_list.append(lat)
        lon_list.append(lon)
        
        
output_file = '../data-experiment2/Locations/airplateGPS.csv'
with open(output_file, 'w', newline='') as file:
    writer = csv.writer(file)
    
    # Write the header (optional)
    writer.writerow(['Latitude', 'Longitude'])
    
    # Iterate over the lists and write each pair as a row
    for item1, item2 in zip(lat_list, lon_list):
        writer.writerow([item1, item2])
