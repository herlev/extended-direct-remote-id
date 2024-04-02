#include "byte_buffer.hpp"
#include "decode.hpp"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"
#include "odid_wifi.h"
#include "opendroneid.h"
#include "routing.hpp"
#include "utils.hpp"
#include <stdio.h>

uint32_t count = 0;

void handle_ethernet_payload(ByteBuffer buffer) {
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

  printf("%u %lu ", odid_msg.msg_counter, count++);
  printf("%02X:%02X:%02X:%02X:%02X:%02X ", frame->sa[0], frame->sa[1],
         frame->sa[2], frame->sa[3], frame->sa[4], frame->sa[5]);
  printf("%016llx ", beacon_frame->timestamp);
  if (odid_msg.uas_data
          .BasicIDValid[0]) { // TODO: maybe loop through all values?
    printf("%s ", odid_msg.uas_data.BasicID[0].UASID);
  }
  if (odid_msg.uas_data.LocationValid) {
    auto &location = odid_msg.uas_data.Location;
    printf("lat: %f, lon: %f, alt: %f", location.Latitude,
    location.Longitude,
           location.AltitudeBaro);
  }
  printf("\n");
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
