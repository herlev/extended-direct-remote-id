#pragma once
#include <cstdint>

// struct ReceiveTypes {
//   bool wifi_nan : 1;
//   bool wifi_beacon : 1;
//   bool bluetooth_legacy : 1;
//   bool bluetooth_5 : 1;
//   bool adsb : 1;
// };

#pragma pack(1)
struct metadata {
  uint8_t options; // 5 used
  uint8_t hop_counter;

  uint32_t ff_uas_id_hash;
  int32_t ff_receive_rssi; // could use 16 bits here instead
  uint32_t ff_lat;
  uint32_t ff_lon;
  uint8_t original_adsb_type;

  // ReceiveTypes receive_bitfield : 8;
  uint8_t data[5];
};

static_assert(sizeof(metadata) == 24, "Size is not correct");
