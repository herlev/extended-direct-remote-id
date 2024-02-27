
# port := "/dev/ttyACM0"

build example="":
	idf.py -DEXAMPLE={{example}} build

flash example="":
	idf.py -DEXAMPLE={{example}} flash

monitor:
	idf.py monitor

# flash and monitor
fm example="": (flash example) monitor
