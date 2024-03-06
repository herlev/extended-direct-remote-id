#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "opendroneid.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "BLE-gap-tx"
uint8_t addr_coded[6] = {0xc0, 0xde, 0x52, 0x00, 0x00, 0x04};
static SemaphoreHandle_t semaphore = NULL;
static esp_ble_gap_ext_adv_params_t ext_adv_params_coded = {
    .type = ESP_BLE_GAP_SET_EXT_ADV_PROP_NONCONN_NONSCANNABLE_UNDIRECTED, // set to unicast advertising
    .interval_min = 160, // T = 0.625 msec * N, where N is interval value. Range: 0x0020 to 0x4000
    .interval_max = 160,
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

extern "C" void app_main(void) {
  esp_err_t ret;

  // Initialize NVS.
  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

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
  while (true) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  return;
}
