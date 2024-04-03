#include "byte_buffer.hpp"
#include "decode.hpp"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "odid_wifi.h"
#include "opendroneid.h"
#include "portmacro.h"
#include "routing.hpp"
#include "utils.hpp"
#include <cstddef>
#include <optional>
#include <stdio.h>
#include <string.h>
#include <sys/_stdint.h>

void handle_ethernet_payload(ByteBuffer buffer) {
  uint8_t mac[] = {0x02, 0x45, 0x6d, 0xff, 0xdd, 0xdd};
  auto frame = (ieee80211_mgmt *)buffer.start();
  if (!is_beacon_frame(*frame)) {
    return;
  }
  if (c_array_equal(frame->sa, mac, 6)) {
    return;
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
    return;
  }
  auto odid_msg_opt = decode_ODID_payload(ODID_payload.value());
  if (!odid_msg_opt.has_value()) {
    return;
  }
  auto odid_msg = odid_msg_opt.value();
  if (!odid_msg.uas_data.BasicIDValid[0]) {
    return;
  }
  uint64_t hash = FNV1_a_hash((uint8_t *)odid_msg.uas_data.BasicID[0].UASID, ODID_ID_SIZE);
  uint8_t count = odid_msg.msg_counter;

  bool found = false;
  for (uint32_t i = 0; i < 1000; i++) {
    if (!table[i].is_valid) {
      continue;
    }
    auto &e = table[i];
    if (e.id == hash) {
      found = true;
      if (e.latest_count < count || count < e.latest_count - 128) {
        e.latest_count = count;
        break;
      } else {
        return;
      }
    }
  }
  if (!found) {
    for (uint32_t i = 0; i < 1000; i++) {
      auto &e = table[i];
      if (!e.is_valid) {
        e.is_valid = true;
        e.latest_count = count;
        e.id = hash;
        break;
      }
    }
  }
  uint8_t buf[1000];
  int len = odid_wifi_build_message_pack_beacon_frame(&odid_msg.uas_data, (char *)mac, "RID", 3, 0x0064,
                                                      odid_msg.msg_counter, buf, 1000);

  if (len < 0) {
    printf("ERROR: %d\n", len);
    return;
  }
  printf("Retransmitting\n");
  ESP_ERROR_CHECK(esp_wifi_80211_tx((wifi_interface_t)0, buf, len, false));
}

void callback(void *buffer, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) { // beacon frames are management frames
    return;
  }
  auto packet = (wifi_promiscuous_pkt_t *)buffer;
  uint8_t fcs_sig_length = 4;
  ByteBuffer bbuf(packet->payload, packet->rx_ctrl.sig_len - fcs_sig_length);
  handle_ethernet_payload(bbuf);
}

extern "C" void app_main(void) {
  initialize();
  while (true) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
