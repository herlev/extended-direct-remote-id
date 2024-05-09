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
#define airplate_id "ARPLF"
struct RssiTableElem {
  uint8_t rssi = 0;
  uint64_t timestamp = 0;
};
uint32_t num_table_elem = 0;
const uint32_t max_table_elem = 12000;
static RssiTableElem rssi_table[max_table_elem] = {};
void printRssiTable() {
  printf("\nRssi table with %lu samples\n", num_table_elem);

  printf("RSSI, timestamp\n\n");
  for (uint32_t i = 0; i < num_table_elem; i++) {
    auto &e = rssi_table[i];
    printf("-%u\t%llu\t\n", e.rssi, e.timestamp);
  }
}

void log_payload(ByteBuffer buffer, signed rssi) {
  uint8_t tx_mac[] = {0x02, 0x45, 0x6d, 0xff, 0xdd, 0xdc};
  auto frame = (ieee80211_mgmt *)buffer.start();
  if (!is_beacon_frame(*frame)) {
    return;
  }

  bool has_HT_control = frame->frame_control & (0b1 << 15);
  uint8_t offset = sizeof(ieee80211_mgmt); // 24
  if (has_HT_control) {
    offset += 4;
  }

  auto beacon_frame = (ieee80211_beacon *)&buffer[offset];
  auto timestamp = beacon_frame->timestamp;
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
  if (num_table_elem >= max_table_elem) {
    return;
  }
  bool is_airplate_valid =
      (strncmp(odid_msg.uas_data.BasicID[0].UASID, airplate_id, 5) == 0 && odid_msg.uas_data.Location.Latitude >= 0.01);
  bool is_transmitter = c_array_equal(frame->sa, tx_mac, 6);
  // Check if Airplate device on UASID. Make sure that latitude is not 0, indicating missing GPS fix. If not airplate
  // check on correct sender MAC
  if (!is_airplate_valid && !is_transmitter) {
    return;
  }
  auto &rssi_table_elem = rssi_table[num_table_elem++];
  rssi_table_elem.timestamp = timestamp;
  rssi_table_elem.rssi = (uint8_t)-1 * rssi;
  return;
}

void callback(void *buffer, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) { // beacon frames are management frames
    return;
  }
  auto packet = (wifi_promiscuous_pkt_t *)buffer;
  uint8_t fcs_sig_length = 4;
  auto rssi = packet->rx_ctrl.rssi;
  ByteBuffer bbuf(packet->payload, packet->rx_ctrl.sig_len - fcs_sig_length);
  log_payload(bbuf, rssi);
}

extern "C" void app_main(void) {
  initialize();

  while (true) {
    int c = getc(stdin);
    if (c == 'p') {
      printRssiTable();
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}
