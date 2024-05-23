#include "byte_buffer.hpp"
#include "decode.hpp"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "utils.hpp"
#include <stdio.h>
uint8_t UUID[] = {0x16, 0xFA, 0xFF};
uint8_t ODID_appcode = 0x0D;
const uint8_t bt5_offset = 5;
uint8_t data_len = 0;
uint8_t *data;
static void gap_ble_ext_scan_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
  switch (event) {
  case ESP_GAP_BLE_SCAN_RESULT_EVT:
    break;
  case ESP_GAP_BLE_EXT_SCAN_START_COMPLETE_EVT:
    printf("Scan start complete");
    break;
  case ESP_GAP_BLE_EXT_ADV_REPORT_EVT:
    if (!(param->ext_adv_report.params.event_type & ESP_BLE_GAP_SET_EXT_ADV_PROP_LEGACY)) { // not legacy BT
      data = param->ext_adv_report.params.adv_data; // Important that we look in the ext_adv_report here.
      data_len = data[0] + 1;                       // length including the byte indicating buffer length
      if (c_array_equal(&data[1], UUID, sizeof(UUID)) && data[4] == ODID_appcode) {
        ByteBuffer odid_buf(&data[bt5_offset], data_len - bt5_offset);
        auto odid_msg_opt = decode_ODID_payload(odid_buf);
        if (!odid_msg_opt.has_value()) {
          printf("could not decode\n");
          return;
        }

        auto odid_msg = odid_msg_opt.value();
        printf("%u ", odid_msg.msg_counter);
        if (odid_msg.uas_data.BasicIDValid[0]) { // TODO: maybe loop through all values?
          printf("%s ", odid_msg.uas_data.BasicID[0].UASID);
        }
        if (odid_msg.uas_data.LocationValid) {
          auto &location = odid_msg.uas_data.Location;
          printf("lat: %f, lon: %f, alt: %f", location.Latitude, location.Longitude, location.AltitudeBaro);
        }
        printf("\n");
      }
    }
    printf("\n");
    break;
  default:
    printf("%s \n", "default case not handled");
    break;
  }
}

static esp_ble_ext_scan_params_t ext_scan_params = {
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE, // BLE_SCAN_DUPLICATE_ENABLE_RESET filter out duplicates but reset
                                                  // duplicate list on each scan interval
    .cfg_mask = ESP_BLE_GAP_EXT_SCAN_CFG_CODE_MASK,
    .uncoded_cfg = {BLE_SCAN_TYPE_ACTIVE, 40, 40},
    .coded_cfg = {BLE_SCAN_TYPE_ACTIVE, 30000, 30000},
};

extern "C" void app_main() {

  // Initialize NVS.
  esp_err_t ret = nvs_flash_init();
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

  ESP_ERROR_CHECK(esp_ble_gap_set_ext_scan_params(&ext_scan_params));

  // register the  callback function to the gap module
  ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_ble_ext_scan_cb));
  ESP_ERROR_CHECK(esp_ble_gap_start_ext_scan(100, 1));

  while (1) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
