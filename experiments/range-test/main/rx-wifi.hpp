#include "byte_buffer.hpp"
#include "decode.hpp"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"
#include "odid_wifi.h"
#include "opendroneid.h"
#include "utils.hpp"
#include <stdio.h>

uint32_t count = 0;

const char *get_rate_name(unsigned rate) {
  switch (rate) {
    case WIFI_PHY_RATE_1M_L: return "1M_L";
    case WIFI_PHY_RATE_2M_L: return "2M_L";
    case WIFI_PHY_RATE_5M_L: return "5M_L";
    case WIFI_PHY_RATE_11M_L: return "11M_L";
    case WIFI_PHY_RATE_2M_S: return "2M_S";
    case WIFI_PHY_RATE_5M_S: return "5M_S";
    case WIFI_PHY_RATE_11M_S: return "11M_S";
    case WIFI_PHY_RATE_48M: return "48M";
    case WIFI_PHY_RATE_24M: return "24M";
    case WIFI_PHY_RATE_12M: return "12M";
    case WIFI_PHY_RATE_6M: return "6M";
    case WIFI_PHY_RATE_54M: return "54M";
    case WIFI_PHY_RATE_36M: return "36M";
    case WIFI_PHY_RATE_18M: return "18M";
    case WIFI_PHY_RATE_9M: return "9M";
    default: return "??";
  }
}

void print_info(ODID_msg odid_msg, wifi_pkt_rx_ctrl_t &rx_ctrl) {
  // noise_floor and rx_state
  printf("%lu\t%u\t%u\t%s\t%d\t%d\t%u\t%u\n", count++, rx_ctrl.timestamp, odid_msg.msg_counter, get_rate_name(rx_ctrl.rate),
         rx_ctrl.rssi, rx_ctrl.noise_floor, rx_ctrl.rx_state, rx_ctrl.sig_len);
}

std::optional<ODID_msg> parse_odid_beacon_msg(ByteBuffer buffer) {
  uint8_t sender_mac[] = {0x02, 0x45, 0x6d, 0xff, 0xdd, 0xdd};
  auto frame = (ieee80211_mgmt *)buffer.start();

  if (!c_array_equal(sender_mac, frame->sa, sizeof(sender_mac))) {
    return {};
  }
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
  return decode_ODID_payload(ODID_payload.value());
}

void callback(void *buffer, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) { // beacon frames are management frames
    return;
  }
  auto packet = (wifi_promiscuous_pkt_t *)buffer;
  uint8_t fcs_sig_length = 4;
  ByteBuffer bbuf(packet->payload, packet->rx_ctrl.sig_len - fcs_sig_length);
  auto odid_msg = parse_odid_beacon_msg(bbuf);
  if (!odid_msg.has_value()) {
    return;
  }
  print_info(odid_msg.value(), packet->rx_ctrl);
}

extern "C" void app_main(void) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&callback));
  ESP_ERROR_CHECK(esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE));

  while (true) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
