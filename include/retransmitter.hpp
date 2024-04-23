#include "decode.hpp"
#include "routing.hpp"
#include "utils.hpp"

void retransmit(ODID_msg odid_msg) {
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
      // printf("routing table full");
      // ESP_ERROR_CHECK(1);
    }

    uint8_t buf[1000];
    int len = odid_wifi_build_message_pack_beacon_frame(&odid_msg.uas_data, (char *)mac, "RID", 3, 0x0064,
                                                        odid_msg.msg_counter, buf, 1000);

    if (len < 0) {
      printf("ERROR: %d\n", len);
      return;
    }
    ESP_ERROR_CHECK(esp_wifi_80211_tx((wifi_interface_t)0, buf, len, false));
  
}

