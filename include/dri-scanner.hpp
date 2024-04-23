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
