#include "decode.hpp"
#include "esp_err.h"
#include "routing.hpp"
#include "utils.hpp"

int table_find(uint64_t hash) {
  for (uint32_t i = 0; i < 1000; i++) {
    auto &e = table[i];
    if (!e.is_valid) {
      continue;
    }
    if (e.id == hash) {
      return i;
    }
  }
  return -1;
}

int table_insert(uint64_t hash, uint8_t count) {
  for (uint32_t i = 0; i < 1000; i++) {
    auto &e = table[i];
    if (!e.is_valid) {
      e.is_valid = true;
      e.latest_count = count;
      e.id = hash;
      return 0;
    }
  }
  return 1;
}

void update_table(int i, uint8_t count) {
  table[i].latest_count = count;
}

void retransmit_dri_message() {
  uint8_t buf[1000];
  int len = odid_wifi_build_message_pack_beacon_frame(&odid_msg.uas_data, (char *)mac, "RID", 3, 0x0064,
                                                      odid_msg.msg_counter, buf, 1000);

  if (len < 0) {
    printf("ERROR: %d\n", len);
    return;
  }
  ESP_ERROR_CHECK(esp_wifi_80211_tx((wifi_interface_t)0, buf, len, false));
}

void retransmit(ScanResult scan_result, uint8_t *own_mac) {
  if (!scan_result.odid_msg.uas_data.BasicIDValid[0]) {
    return;
  }
  if (c_array_equal(scan_result.management_frame.sa, own_mac, 6)) {
    return;
  }
  uint64_t hash = FNV1_a_hash((uint8_t *)scan_result.odid_msg.uas_data.BasicID[0].UASID, ODID_ID_SIZE);
  uint8_t count = scan_result.odid_msg.msg_counter;

  int table_elem_i = table_find(hash);

  if (table_elem_i == -1) {
    ESP_ERROR_CHECK(table_insert(hash, count));
    retransmit_dri_message();
    return;
  }
  if (is_newer(table[table_elem_i].latest_count, count)) {
    update_table(table_elem_i, count);
    retransmit_dri_message();
  }
}
