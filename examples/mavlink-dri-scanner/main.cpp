#include "all/mavlink.h"
#include "byte_buffer.hpp"
#include "common/mavlink_msg_adsb_vehicle.h"
#include "decode.hpp"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"
#include "mavlink_helpers.h"
#include "mavlink_types.h"
#include "odid_wifi.h"
#include "opendroneid.h"
#include "routing.hpp"
#include "utils.hpp"
#include <stdio.h>

const int32_t LATLON_MULT = 10000000;
static int intRangeMax(int64_t inValue, int startRange, int endRange) {
  if (inValue < startRange) {
    return startRange;
  } else if (inValue > endRange) {
    return endRange;
  } else {
    return (int)inValue;
  }
}
static int32_t encodeLatLon(double LatLon_data) {
  return (int32_t)intRangeMax((int64_t)(LatLon_data * LATLON_MULT), -180 * LATLON_MULT, 180 * LATLON_MULT);
}
void send_adsb(uart_port_t uart_num, uint8_t system_id, uint8_t component_id, uint8_t channel, char *callsign,
               uint32_t icao, uint32_t lat, uint32_t lon, uint8_t emitter_type) {
  mavlink_message_t msg;
  uint8_t buf[MAVLINK_MAX_PACKET_LEN] = {};

  int len = mavlink_msg_adsb_vehicle_pack(system_id, component_id, &msg, icao, lat, lon, 1, 30000, 0, 0, 0, callsign,
                                          emitter_type, 1, 0b11111, 6969);
  int l = mavlink_msg_to_send_buffer(buf, &msg);
  uart_write_bytes(uart_num, buf, l);
}

const uart_port_t uart_num = UART_NUM_1;
uart_config_t uart_config = {
    .baud_rate = 2000000,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = 122,
};
const int uart_buffer_size = (1024 * 2);
QueueHandle_t uart_queue;
uint8_t rx_data[128];

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

  if (odid_msg.uas_data.LocationValid && odid_msg.uas_data.BasicIDValid[0]) {
    auto lat = encodeLatLon(odid_msg.uas_data.Location.Latitude);
    auto lon = encodeLatLon(odid_msg.uas_data.Location.Longitude);
    auto callsign = odid_msg.uas_data.BasicID->UASID;
    auto emitter_type = ADSB_EMITTER_TYPE_NO_INFO;
    uint32_t icao = FNV1_a_hash((uint8_t *)callsign, strlen(callsign));
    if (odid_msg.uas_data.BasicID->UAType == ODID_UATYPE_HELICOPTER_OR_MULTIROTOR) {
      emitter_type = ADSB_EMITTER_TYPE_UAV;
    }
    send_adsb(uart_num, 1, 156, 5, callsign, icao, lat, lon, emitter_type);
  }
  printf("%02X:%02X:%02X:%02X:%02X:%02X ", frame->sa[0], frame->sa[1], frame->sa[2], frame->sa[3], frame->sa[4],
         frame->sa[5]);
  printf("%016llx ", beacon_frame->timestamp);
  if (odid_msg.uas_data.BasicIDValid[0]) { // TODO: maybe loop through all values?
    printf("%s ", odid_msg.uas_data.BasicID[0].UASID);
  }
  if (odid_msg.uas_data.LocationValid) {
    auto &location = odid_msg.uas_data.Location;
    printf("lat: %f, lon: %f, alt: %f", location.Latitude, location.Longitude, location.AltitudeBaro);
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

  ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config)); // Init uart
  ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, 21, 20, 6, 7));
  ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, uart_buffer_size, uart_buffer_size, 100, &uart_queue, 0));

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
