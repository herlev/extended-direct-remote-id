
port := `./get-ttyacm-port.sh`

build example="":
	idf.py -DEXAMPLE={{example}} build

flash example="":
	idf.py -DEXAMPLE={{example}} -p {{port}} flash

monitor:
	idf.py monitor -p {{port}}

# flash and monitor
fm example="": (flash example) monitor
