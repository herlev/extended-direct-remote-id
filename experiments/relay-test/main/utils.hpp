#pragma once
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include <string.h>

static bool c_array_equal(void *a, void *b, size_t len) {
  return memcmp(a, b, len) == 0;
}

bool is_newer(uint8_t saved_count, uint8_t count) {
  return count > saved_count || count < saved_count - 128;
}

void callback(void *buffer, wifi_promiscuous_pkt_type_t type);

static void initialize() {
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

  // ram
  // esp_base_mac_addr_set
  // esp_wifi_set_bandwidth
  // esp_wifi_set_max_tx_power
  // esp_wifi_set_protocol
  // esp_wifi_set_channel
  // uint8_t buf[] = {0x}

  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&callback));
  ESP_ERROR_CHECK(esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE));
}
