// Define Beacon TX to make this device broadcast a ODID package with increasing message counter. Frequency of 10Hz
// If not defined the device will act as a relaying device
#define BeaconTX
#ifdef BeaconTX
#include "beacon-tx.hpp"
#else
#include "beacon-relay.hpp"
#endif
