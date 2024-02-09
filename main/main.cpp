#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "odid_wifi.h"
#include "opendroneid.h"
#include "portmacro.h"
#include <cstddef>
#include <optional>
#include <stdio.h>
#include <string.h>
#include <sys/_stdint.h>

struct ODID_payload {
  uint8_t *buf;
  uint32_t len;
};

bool c_array_equal(void *a, void *b, size_t len) {
  return memcmp(a, b, len) == 0;
}

std::optional<ODID_payload> get_ODID_payload(uint8_t *tagged_params,
                                             uint32_t len) {
  uint8_t vendor_specific_type = 0xdd;
  uint8_t CEN_OUI[3] = {0xFA, 0x0B, 0xBC};
  uint8_t OUI_len = 3;

  for (uint32_t offset = 0; offset < len;) {
    auto *tag = (ieee80211_vendor_specific *)&tagged_params[offset];
    uint8_t header_size = sizeof(ieee80211_vendor_specific);

    if (tag->element_id == vendor_specific_type && offset + header_size < len &&
        c_array_equal(tag->oui, CEN_OUI, OUI_len)) {
      return ODID_payload{.buf = &tagged_params[offset + header_size],
                          .len = tag->length - 4u}; // size of oui+oui_type = 4
    }
    offset += tag->length + 2; // size of element_id+length = 2
  }
  return {};
}

bool is_beacon_frame(ieee80211_mgmt &frame) {
  uint16_t beacon_type =
      0x0080; // https://en.wikipedia.org/wiki/802.11_frame_types
  return (frame.frame_control & 0x00FC) == beacon_type;
}

struct ODID_msg {
  uint8_t msg_counter;
  ODID_UAS_Data uas_data;
};

std::optional<ODID_msg> decode_ODID_payload(ODID_payload &payload) {
  ODID_UAS_Data UAS_data;
  uint8_t msg_counter = payload.buf[0];
  uint8_t *msg_pack = payload.buf + 1;
  uint8_t msg_pack_len = payload.len - 1;
  if (odid_message_process_pack(&UAS_data, msg_pack, msg_pack_len) < 0) {
    return {};
  }
  return ODID_msg{.msg_counter = msg_counter, .uas_data = UAS_data};
}

void callback(void *buffer, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) { // beacon frames are management frames
    return;
  }
  auto packet = (wifi_promiscuous_pkt_t *)buffer;
  uint8_t *payload = packet->payload;
  auto frame = (ieee80211_mgmt *)payload;
  if (!is_beacon_frame(*frame)) {
    return;
  }

  bool has_HT_control = frame->frame_control & (0b1 << 15);
  uint8_t offset = sizeof(ieee80211_mgmt); // 24
  if (has_HT_control) {
    offset += 4;
  }
  uint8_t fcs_length = 4;

  auto beacon_frame = (ieee80211_beacon *)&payload[offset];
  auto beacon_frame_tagged_params = &payload[offset + sizeof(ieee80211_beacon)];
  auto tagged_params_len =
      packet->rx_ctrl.sig_len - offset - sizeof(ieee80211_beacon) - fcs_length;

  auto ODID_payload =
      get_ODID_payload(beacon_frame_tagged_params, tagged_params_len);

  if (!ODID_payload.has_value()) {
    return;
  }
  auto odid_msg_opt = decode_ODID_payload(ODID_payload.value());
  if (!odid_msg_opt.has_value()) {
    return;
  }
  auto odid_msg = odid_msg_opt.value();

  printf("%u ", odid_msg.msg_counter);
  if (odid_msg.uas_data
          .BasicIDValid[0]) { // TODO: maybe loop through all values?
    printf("%s ", odid_msg.uas_data.BasicID[0].UASID);
  }
  if (odid_msg.uas_data.LocationValid) {
    auto &location = odid_msg.uas_data.Location;
    printf("lat: %f, lon: %f, alt: %f", location.Latitude, location.Longitude,
           location.AltitudeBaro);
  }
  printf("\n");
}

extern "C" void app_main(void) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
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
    printf("Hello world\n");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
