#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"
#include "opendroneid.h"
#include "utils.hpp"
#include <stdio.h>

ODID_UAS_Data get_odid_data() {
  ODID_UAS_Data data;
  odid_initUasData(&data);
  data.BasicIDValid[0] = true;
  data.BasicID[0].IDType = ODID_IDTYPE_SERIAL_NUMBER;
  data.BasicID[0].UAType = ODID_UATYPE_ROCKET;
  char uasid[] = "EDRI";
  strcpy(data.BasicID[0].UASID, uasid);
  data.LocationValid = 1;
  data.SystemValid = 1;
  data.OperatorIDValid = 1;
  data.SelfIDValid = 1;
  return data;
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
  ESP_ERROR_CHECK(esp_wifi_config_80211_tx_rate(WIFI_IF_STA, WIFI_PHY_RATE_1M_L));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE));

  uint8_t count = 0;
  uint8_t mac[] = {0x02, 0x45, 0x6d, 0xff, 0xdd, 0xdc};
  uint8_t buf[1000];
  ODID_UAS_Data data = get_odid_data();
  while (true) {
    int len = odid_wifi_build_message_pack_beacon_frame(&data, (char *)mac, "RID", 3, 0x0064, count++, buf, 1000);
    if (len < 0) {
      printf("ERROR: %d\n", len);
      continue;
    }
    ESP_ERROR_CHECK(esp_wifi_80211_tx((wifi_interface_t)0, buf, len, false));
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}
