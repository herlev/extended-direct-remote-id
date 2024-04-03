#pragma once
#include "byte_buffer.hpp"
#include "odid_wifi.h"
#include "utils.hpp"
#include <optional>
#include <sys/_stdint.h>

static std::optional<ByteBuffer> get_ODID_payload(uint8_t *tagged_params, uint32_t len) {
  uint8_t vendor_specific_type = 0xdd;
  uint8_t CEN_OUI[3] = {0xFA, 0x0B, 0xBC};
  uint8_t OUI_len = 3;

  for (uint32_t offset = 0; offset < len;) {
    auto *tag = (ieee80211_vendor_specific *)&tagged_params[offset];
    uint8_t header_size = sizeof(ieee80211_vendor_specific);

    if (tag->element_id == vendor_specific_type && offset + header_size < len &&
        c_array_equal(tag->oui, CEN_OUI, OUI_len)) {
      return ByteBuffer(&tagged_params[offset + header_size], tag->length - 4u); // size of oui+oui_type = 4
    }
    offset += tag->length + 2; // size of element_id+length = 2
  }
  return {};
}

static bool is_beacon_frame(ieee80211_mgmt &frame) {
  uint16_t beacon_type = 0x0080; // https://en.wikipedia.org/wiki/802.11_frame_types
  return (frame.frame_control & 0x00FC) == beacon_type;
}

struct ODID_msg {
  uint8_t msg_counter;
  ODID_UAS_Data uas_data;
};

static std::optional<ODID_msg> decode_ODID_payload(ByteBuffer &payload) {
  ODID_UAS_Data UAS_data;
  uint8_t msg_counter = payload[0];
  ByteBuffer msg_pack = payload.sub_buffer(1);
  auto ret = odid_message_process_pack(&UAS_data, msg_pack.start(), msg_pack.length());
   if (ret < 0) {
    // printf("Error from decode function: %d\n",ret);
    return {};

  }
  return ODID_msg{.msg_counter = msg_counter, .uas_data = UAS_data};
}
