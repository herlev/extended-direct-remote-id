#include "byte_buffer.hpp"
#include "decode.hpp"
#include "driver/ledc.h"
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
const uint16_t duty_2 = 164;
const uint16_t duty_125 = 1024;
float map(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
static void example_ledc_init(void) {
  // Prepare and then apply the LEDC PWM timer configuration
  ledc_timer_config_t ledc_timer = {.speed_mode = LEDC_LOW_SPEED_MODE,
                                    .duty_resolution = LEDC_TIMER_13_BIT,
                                    .timer_num = LEDC_TIMER_0,
                                    .freq_hz = 50, // Set output frequency at 4 kHz
                                    .clk_cfg = LEDC_AUTO_CLK};
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  // Prepare and then apply the LEDC PWM channel configuration
  ledc_channel_config_t ledc_channel = {.gpio_num = GPIO_NUM_8,
                                        .speed_mode = LEDC_LOW_SPEED_MODE,
                                        .channel = LEDC_CHANNEL_0,
                                        .intr_type = LEDC_INTR_DISABLE,
                                        .timer_sel = LEDC_TIMER_0,
                                        .duty = 0, // Set duty to 0%
                                        .hpoint = 0};
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}
uint32_t calculate_duty(float duty) {

  return map(duty, 2.0, 12.5, duty_2, duty_125);
}

float angle_to_duty(uint8_t angle_deg) {
  return map(float(angle_deg), 0.0, 180.0, 2.0, 12.5);
}
struct RssiTableElem {
  uint8_t rssi = 0;
  uint64_t timestamp = 0;
};
uint32_t num_table_elem = 0;
const uint32_t max_table_elem = 12000;
static RssiTableElem rssi_table[max_table_elem] = {};
void printRssiTable() {
  // printf("\nRssi table with %lu samples\n", num_table_elem);
  // printf("RSSI, timestamp\n\n");
  for (uint32_t i = 0; i < num_table_elem; i++) {
    auto &e = rssi_table[i];
    printf("-%u\t%llu\n", e.rssi, e.timestamp);
  }
}

void log_payload(ByteBuffer buffer, signed rssi) {
  uint8_t tx_mac[] = {0x02, 0x45, 0x6d, 0xff, 0xdd, 0xdc};
  auto frame = (ieee80211_mgmt *)buffer.start();
  if (!is_beacon_frame(*frame)) {
    return;
  }
  if (!c_array_equal(frame->sa, tx_mac, 6)) {
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
  example_ledc_init();
  uint8_t angle_stepsize = 5;
  uint16_t receive_amount = 100;
  uint32_t timer_val = 0;
  float duty = 0.0;

  duty = angle_to_duty(0);
  timer_val = calculate_duty(duty);
  ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, timer_val));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  for (uint8_t i = 0; i <= 180; i += angle_stepsize) {
    duty = angle_to_duty(i);
    timer_val = calculate_duty(duty);
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, timer_val));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
    vTaskDelay(500 / portTICK_PERIOD_MS);
    num_table_elem = 0;
    while (num_table_elem < receive_amount) {
      vTaskDelay(1);
    }
    printf("Angle: %d\n", i);
    printRssiTable();
    printf("\n\n");
  }
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  duty = angle_to_duty(0);
  timer_val = calculate_duty(duty);
  ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, timer_val));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
  // printf("Done\n");
  while (true) {
    vTaskDelay(1);
  }
}
