#include "adsb.hpp"
#include "dri-scanner.hpp"
#include "esp_mac.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "mavlink-uart.hpp"
#include "portmacro.h"
#include "routing.hpp"
#include "utils.hpp"
#include <sys/_stdint.h>
#define UART_TX GPIO_NUM_21
#define UART_RX GPIO_NUM_20
#define baud 2000000
#define uas_id "EDRI-Relay"
uint8_t uart_rx_buffer[MAVLINK_MAX_PACKET_LEN] = {};
uint8_t m_buffer[MAVLINK_MAX_PACKET_LEN] = {};
uint8_t mac[] = {};
#include "retransmitter.hpp"

static uint8_t send_buffer_self[1000];

void print_mac(uint8_t *mac) {
  printf("%02X:%02X:%02X:%02X:%02X:%02X \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}
void print_scan_result(ScanResult &scan_result) {
  auto &odid_msg = scan_result.odid_msg;
  auto &frame = scan_result.management_frame;
  auto &beacon_frame = scan_result.beacon_frame;
  printf("%u ", odid_msg.msg_counter);
  printf("%02X:%02X:%02X:%02X:%02X:%02X ", frame.sa[0], frame.sa[1], frame.sa[2], frame.sa[3], frame.sa[4],
         frame.sa[5]);
  printf("%016llx ", beacon_frame.timestamp);
  if (odid_msg.uas_data.BasicIDValid[0]) { // TODO: maybe loop through all values?
    printf("%s ", odid_msg.uas_data.BasicID[0].UASID);
  }
  if (odid_msg.uas_data.LocationValid) {
    auto &location = odid_msg.uas_data.Location;
    printf("lat: %f, lon: %f, alt: %f", location.Latitude, location.Longitude, location.AltitudeGeo);
  }
  printf("\n");
}

constexpr uart_port_t uart_port = UART_NUM_1;
extern "C" void app_main(void) {
  uint8_t msg_counter = 0;
  initialize();
  esp_read_mac(mac, ESP_MAC_WIFI_STA);

  initialize_uart(uart_port, UART_TX, UART_RX, baud);

  scan_callback = [](ScanResult scan_result) {
    retransmit(scan_result, mac);
    print_scan_result(scan_result);
    inject_adsb(scan_result.odid_msg.uas_data, uart_port,uas_id);
  };

  uint32_t t_prev = (uint32_t)esp_timer_get_time() / 1000;
  uint32_t t_now = t_prev;
  while (true) {
    t_now = (uint32_t)esp_timer_get_time() / 1000;
    if (t_now - t_prev > 5000) {
      t_prev = t_now;
      initialize_data_stream(uart_port, MAVLINK_MSG_ID_GLOBAL_POSITION_INT, 100000, m_buffer);
    }
    update_dri_mavlink(uart_port, uart_rx_buffer, send_buffer_self, uas_id, (char *)mac, msg_counter++);

    vTaskDelay(1);
  }
}
