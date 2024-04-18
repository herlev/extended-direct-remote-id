#pragma once
#include "decode.hpp"
#include "odid_wifi.h"
#include <cstddef>

struct ScanResult_ {
  ieee80211_mgmt &management_frame;
  ieee80211_beacon &beacon_frame;
  // ODID_msg odid_msg;
};

struct ScanResult {
  ieee80211_mgmt &management_frame;
  ieee80211_beacon &beacon_frame;
  ODID_msg &odid_msg;
};

//   std::optional<ScanResult> parse_ethernet_payload(ByteBuffer buffer) {
//     uint8_t mac[] = {0x02, 0x45, 0x6d, 0xff, 0xdd, 0xdd};
//     auto frame = (ieee80211_mgmt *)buffer.start();
//     if (!is_beacon_frame(*frame)) {
//       return {};
//     }
//     if (c_array_equal(frame->sa, mac, 6)) {
//       return {};
//     }

//     bool has_HT_control = frame->frame_control & (0b1 << 15);
//     uint8_t offset = sizeof(ieee80211_mgmt); // 24
//     if (has_HT_control) {
//       offset += 4;
//     }

//     auto beacon_frame = (ieee80211_beacon *)&buffer[offset];
//     auto beacon_frame_tagged_params = &buffer[offset + sizeof(ieee80211_beacon)];
//     auto tagged_params_len = buffer.length() - offset - sizeof(ieee80211_beacon);

//     auto ODID_payload = get_ODID_payload(beacon_frame_tagged_params, tagged_params_len);

//     if (!ODID_payload.has_value()) {
//       return {};
//     }
//     auto odid_msg_opt = decode_ODID_payload(ODID_payload.value());
//     if (!odid_msg_opt.has_value()) {
//       return {};
//     }
//     auto odid_msg = odid_msg_opt.value();
//     return ScanResult{.management_frame = *frame, .beacon_frame = *beacon_frame, .odid_msg = odid_msg};

//     // printf("%u ", odid_msg.msg_counter);
//     // printf("%02X:%02X:%02X:%02X:%02X:%02X ", frame->sa[0], frame->sa[1],
//     //        frame->sa[2], frame->sa[3], frame->sa[4], frame->sa[5]);
//     // printf("%016llx ", beacon_frame->timestamp);
//     // if (odid_msg.uas_data
//     //         .BasicIDValid[0]) { // TODO: maybe loop through all values?
//     //   printf("%s ", odid_msg.uas_data.BasicID[0].UASID);
//     // }
//     // if (odid_msg.uas_data.LocationValid) {
//     //   auto &location = odid_msg.uas_data.Location;
//     //   printf("lat: %f, lon: %f, alt: %f", location.Latitude,
//     //   location.Longitude,
//     //          location.AltitudeBaro);
//     // }
//     // printf("\n");

//     // if (!odid_msg.uas_data.BasicIDValid[0]) {
//     //   return;
//     // }
//     // uint64_t hash = FNV1_a_hash((uint8_t *)odid_msg.uas_data.BasicID[0].UASID, ODID_ID_SIZE);
//     // uint8_t count = odid_msg.msg_counter;

//     // bool found = false;
//     // for (uint32_t i = 0; i < 1000; i++) {
//     //   if (!table[i].is_valid) {
//     //     continue;
//     //   }
//     //   auto &e = table[i];
//     //   if (e.id == hash) {
//     //     found = true;
//     //     if (e.latest_count < count || count < e.latest_count - 128) {
//     //       e.latest_count = count;
//     //       break;
//     //     } else {
//     //       return;
//     //     }
//     //   }
//     // }
//     // if (!found) {
//     //   for (uint32_t i = 0; i < 1000; i++) {
//     //     auto &e = table[i];
//     //     if (!e.is_valid) {
//     //       e.is_valid = true;
//     //       e.latest_count = count;
//     //       e.id = hash;
//     //       break;
//     //     }
//     //   }
//     //   // printf("routing table full");
//     //   // ESP_ERROR_CHECK(1);
//     // }

//     // uint8_t buf[1000];
//     // int len = odid_wifi_build_message_pack_beacon_frame(&odid_msg.uas_data, (char *)mac, "RID", 3, 0x0064,
//     //                                                     odid_msg.msg_counter, buf, 1000);

//     // if (len < 0) {
//     //   printf("ERROR: %d\n", len);
//     //   return;
//     // }
//     // ESP_ERROR_CHECK(esp_wifi_80211_tx((wifi_interface_t)0, buf, len, false));
//   }

// statically allocated as it takes up 928 bytes
// and it is not possible to change the stack
// size of the wifi task in menuconfig
static ODID_msg odid_msg;

std::optional<ScanResult_> parse_ethernet_payload(ByteBuffer &buffer) {
  auto frame = (ieee80211_mgmt *)buffer.start();
  if (!is_beacon_frame(*frame)) {
    return {};
  }

  bool has_HT_control = frame->frame_control & (0b1 << 15);
  uint8_t offset = sizeof(ieee80211_mgmt); // 24
  if (has_HT_control) {
    offset += 4;
  }

  auto beacon_frame = (ieee80211_beacon *)&buffer[offset];
  auto beacon_frame_tagged_params = &buffer[offset + sizeof(ieee80211_beacon)];
  auto tagged_params_len = buffer.length() - offset - sizeof(ieee80211_beacon);

  auto ODID_payload = get_ODID_payload(beacon_frame_tagged_params, tagged_params_len);

  if (!ODID_payload.has_value()) {
    return {};
  }
  auto odid_msg_opt = decode_ODID_payload(ODID_payload.value());
  if (!odid_msg_opt.has_value()) {
    return {};
  }
  odid_msg = odid_msg_opt.value();
  return ScanResult_{.management_frame = *frame, .beacon_frame = *beacon_frame /*, .odid_msg = odid_msg*/};
}

using cb_t = void (*)(ScanResult);

static cb_t scan_callback = nullptr;

void callback(void *buffer, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) { // beacon frames are management frames
    return;
  }
  auto packet = (wifi_promiscuous_pkt_t *)buffer;
  uint8_t fcs_sig_length = 4;
  ByteBuffer bbuf(packet->payload, packet->rx_ctrl.sig_len - fcs_sig_length);
  auto scan_result_opt = parse_ethernet_payload(bbuf);

  if (!scan_result_opt.has_value()) {
    return;
  }

  auto scan_result = scan_result_opt.value();

  if (scan_callback != nullptr) {
    scan_callback(ScanResult{.management_frame = scan_result.management_frame,
                             .beacon_frame = scan_result.beacon_frame,
                             .odid_msg = odid_msg});
  }
}
