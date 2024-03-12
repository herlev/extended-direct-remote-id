#include "driver/gpio.h"
#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_err.h"
#include "esp_gap_ble_api.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "nvs_flash.h"
#include "opendroneid.h"
#include <stdio.h>
#define LOG_TAG "BLE-gap-tx"
static SemaphoreHandle_t semaphore = NULL;
static esp_ble_gap_ext_adv_t ext_adv[1] = {
    // instance, duration, peroid
    [0] = {0, 0, 0},
};
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
  switch (event) {
  case ESP_GAP_BLE_EXT_ADV_SET_RAND_ADDR_COMPLETE_EVT:
    xSemaphoreGive(semaphore);
    ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_EXT_ADV_SET_RAND_ADDR_COMPLETE_EVT, status %d", param->ext_adv_set_rand_addr.status);
    break;
  case ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT:
    xSemaphoreGive(semaphore);
    ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT, status %d", param->ext_adv_set_params.status);
    break;
  case ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT:
    xSemaphoreGive(semaphore);
    ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT, status %d", param->ext_adv_data_set.status);
    break;
  case ESP_GAP_BLE_EXT_SCAN_RSP_DATA_SET_COMPLETE_EVT:
    xSemaphoreGive(semaphore);
    ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_EXT_SCAN_RSP_DATA_SET_COMPLETE_EVT, status %d", param->scan_rsp_set.status);
    break;
  case ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT:
    xSemaphoreGive(semaphore);
    ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT, status %d", param->ext_adv_start.status);
    break;
  case ESP_GAP_BLE_EXT_ADV_STOP_COMPLETE_EVT:
    xSemaphoreGive(semaphore);
    ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_EXT_ADV_STOP_COMPLETE_EVT, status %d", param->ext_adv_stop.status);
    break;
  default:
    break;
  }
}
void init_BT() {

  uint8_t addr_coded[6] = {0xc0, 0xde, 0x52, 0x00, 0x00, 0x04};
  static esp_ble_gap_ext_adv_params_t ext_adv_params_coded = {
      .type = ESP_BLE_GAP_SET_EXT_ADV_PROP_NONCONN_NONSCANNABLE_UNDIRECTED, // set to unicast advertising
      .interval_min = 1600, // T = 0.625 msec * N, where N is interval value. Range: 0x0020 to 0x4000
      .interval_max = 1600, // 160 = 10 Hz, 1600 = 1 Hz
      .channel_map = ADV_CHNL_ALL,
      .own_addr_type = BLE_ADDR_TYPE_RANDOM,
      .filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_WLST, // we want unicast non-connectable transmission
      .tx_power =
          126, // Previously: EXT_ADV_TX_PWR_NO_PREFERENCE. Now max power allowed. tx power, range: [-127, +126] dBm
      .primary_phy = ESP_BLE_GAP_PHY_CODED,
      .max_skip = 0,
      .secondary_phy = ESP_BLE_GAP_PHY_CODED,
      .sid = 1,
      .scan_req_notif = false,
  };

  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));

  ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
  esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_bluedroid_init_with_cfg(&bluedroid_cfg));

  ESP_ERROR_CHECK(esp_bluedroid_enable());
  ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));

  semaphore = xSemaphoreCreateBinary();

  ESP_ERROR_CHECK(esp_ble_gap_ext_adv_set_params(0, &ext_adv_params_coded));
  xSemaphoreTake(semaphore, portMAX_DELAY);
  ESP_ERROR_CHECK(esp_ble_gap_ext_adv_set_rand_addr(0, addr_coded));
  xSemaphoreTake(semaphore, portMAX_DELAY);
};
static uint8_t ext_adv_inst = 0;
void deinit_BT() {
  ESP_ERROR_CHECK(esp_ble_gap_ext_adv_stop(1, &ext_adv_inst));
  xSemaphoreTake(semaphore, portMAX_DELAY);
}

void transmit_BT(uint16_t transmit_time_ms) {
  uint8_t buf[1000];
  uint8_t bt5_header[] = {0x00, 0x16, 0xFA, 0xFF, 0x0d, 0x00};
  uint8_t bt5_header_len = sizeof(bt5_header);
  uint8_t msg_counter_idx = 5;
  memcpy(buf, bt5_header, bt5_header_len);

  ODID_UAS_Data data;
  odid_initUasData(&data);
  data.BasicIDValid[0] = true;
  data.BasicID[0].IDType = ODID_IDTYPE_SERIAL_NUMBER;
  data.BasicID[0].UAType = ODID_UATYPE_ROCKET;
  char uasid[] = "EDRI";
  strcpy(data.BasicID[0].UASID, uasid);
  int len = odid_message_build_pack(&data, &buf[bt5_header_len], 1000 - bt5_header_len);
  buf[msg_counter_idx] = 0; // msg counter
  buf[0] = len + bt5_header_len - 1;
  int tx_len = len + bt5_header_len;

  ESP_ERROR_CHECK(esp_ble_gap_config_ext_adv_data_raw(0, tx_len, &buf[0]));
  xSemaphoreTake(semaphore, portMAX_DELAY);
  ESP_ERROR_CHECK(esp_ble_gap_ext_adv_start(1, &ext_adv[0]));
  xSemaphoreTake(semaphore, portMAX_DELAY);
  vTaskDelay(transmit_time_ms / portTICK_PERIOD_MS);
}

void transmit_beacon(wifi_phy_rate_t bitrate, uint16_t interval, uint16_t num_packages) {
  wifi_interface_t interface = WIFI_IF_STA;
  ESP_ERROR_CHECK(esp_wifi_stop());
  esp_wifi_config_80211_tx_rate(interface, bitrate);
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE));
  uint8_t mac[] = {0x02, 0x45, 0x6d, 0xff, 0xdd, 0xdd};
  ODID_UAS_Data data;
  odid_initUasData(&data);
  data.BasicIDValid[0] = true;
  data.BasicID[0].IDType = ODID_IDTYPE_SERIAL_NUMBER;
  data.BasicID[0].UAType = ODID_UATYPE_ROCKET;
  char uasid[] = "EDRI_Experiment";
  strcpy(data.BasicID[0].UASID, uasid);

  uint8_t buf[1000];
  for (int i = 0; i < num_packages; i++) {
    int len = odid_wifi_build_message_pack_beacon_frame(&data, (char *)mac, "RID", 3, 0x0064, i, buf, 1000);
    if (len < 0) {
      printf("ERROR: %d\n", len);
    }

    ESP_ERROR_CHECK(esp_wifi_80211_tx(interface, buf, len, false));
    vTaskDelay(interval / portTICK_PERIOD_MS);
  }
}

void init_button() {
  const gpio_num_t pin = GPIO_NUM_2;
  const gpio_config_t pin_conf = {
      .pin_bit_mask = 0b1 << pin,
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&pin_conf));
}
void wait_button() {
  const gpio_num_t pin = GPIO_NUM_2;
  while (gpio_get_level(pin)) {
    vTaskDelay(10);
  }
  while (!gpio_get_level(pin)) {
    vTaskDelay(10);
  }
}

enum class state { BT5, Beacon_1M, Beacon_12M, Beacon_24M, Beacon_54M };

extern "C" void app_main(void) {
  esp_err_t ret;
  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  init_button();

  state x = state::Beacon_1M;
  while (true) {
    switch (x) {
    case state::BT5:
      init_BT();
      transmit_BT(10000);
      deinit_BT();
      wait_button();
      x = state::Beacon_1M;
      break;
    case state::Beacon_1M:
      printf("state beacon 1 Mbit/s press button to start transmitting\n");
      wait_button();
      transmit_beacon(WIFI_PHY_RATE_1M_L, 50, 25);
      printf("done transmitting \n");
      x = state::Beacon_12M;
      break;
    case state::Beacon_12M:
      printf("state beacon 12 Mbit/s press button to start transmitting\n");
      wait_button();
      transmit_beacon(WIFI_PHY_RATE_12M, 50, 25);
      printf("done transmitting \n");
      x = state::Beacon_24M;
      break;
    case state::Beacon_24M:
      printf("state beacon 24 Mbit/s press button to start transmitting\n");
      wait_button();
      transmit_beacon(WIFI_PHY_RATE_24M, 50, 25);
      printf("done transmitting \n");
      x = state::Beacon_54M;
      break;
    case state::Beacon_54M:
      printf("state beacon 54 Mbit/s press button to start transmitting\n");
      wait_button();
      transmit_beacon(WIFI_PHY_RATE_54M, 50, 25);
      printf("done transmitting \n");
      // deinit wifi
      break;
    default:
      break;
    }
  }
}
