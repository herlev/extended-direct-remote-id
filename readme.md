## Developing

Install esp-idf as described [here](https://docs.espressif.com/projects/esp-idf/en/release-v5.3/esp32/get-started/linux-macos-setup.html).

### Build
To build the main application, run:
`idf.py build`

### Flash
To flash the main application, run:
`idf.py -p <port> flash`

### Flash an example

The subdirectory `examples` contains various example applications. To flash these run the following command:

`idf.py -DEXAMPLE=<example_name> -p <port> flash`

### Experiments

The files for the experiments performed are available under `experiments`.
