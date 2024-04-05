
port := `./get-ttyacm-port.sh || true`

[private]
assert_port_set:
	@[[ -n "{{port}}" ]] || exit 1 

build example="":
	idf.py -DEXAMPLE={{example}} build

flash example="": assert_port_set
	idf.py -DEXAMPLE={{example}} -p {{port}} flash

monitor: assert_port_set
	idf.py monitor -p {{port}}

# flash and monitor
fm example="": (flash example) monitor
