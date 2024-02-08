#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "portmacro.h"
#include <stdio.h>

void callback(void *buffer, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) { // beacon frames are management frames
    return;
  } 
  auto packet = (wifi_promiscuous_pkt_t *) buffer;
  auto payload = packet->payload;
  auto frame_type = payload[0]; // https://en.wikipedia.org/wiki/IEEE_802.11#Management_frames
  auto beacon_type = 0x80; // https://en.wikipedia.org/wiki/802.11_frame_types
  if (frame_type != beacon_type) {
    return;
  }
  // 0x80 0x40 0x50

  printf("received packet with length %d\n", packet->rx_ctrl.sig_len);
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
