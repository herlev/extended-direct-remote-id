#include "dri-scanner.hpp"
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
#define uas_id "bobby"
uint8_t rx_data[MAVLINK_MAX_PACKET_LEN] = {};
uint8_t m_buffer[MAVLINK_MAX_PACKET_LEN] = {};
uint8_t mac[] = {0x02, 0x45, 0x6d, 0xff, 0xdd, 0xdc};
#include "retransmitter.hpp"

static uint8_t send_buffer_self[1000];

void print_scan_result(ScanResult &scan_result) {
    auto &odid_msg = scan_result.odid_msg;
    auto &frame = scan_result.management_frame;
    auto &beacon_frame = scan_result.beacon_frame;
    printf("%u ", odid_msg.msg_counter);
    printf("%02X:%02X:%02X:%02X:%02X:%02X ", frame.sa[0], frame.sa[1],
           frame.sa[2], frame.sa[3], frame.sa[4], frame.sa[5]);
    printf("%016llx ", beacon_frame.timestamp);
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

extern "C" void app_main(void) {
  uint8_t msg_counter = 0;
  initialize();
  uart_port_t uart_port = UART_NUM_1;
  initialize_uart(uart_port, UART_TX, UART_RX, baud);

  scan_callback = [](ScanResult scan_result) {
    retransmit(scan_result.odid_msg);
    print_scan_result(scan_result);
  };

  uint32_t t_prev = (uint32_t)esp_timer_get_time() / 1000;
  uint32_t t_now = t_prev;
  while (true) {
    if (t_now - t_prev > 10000) {
      t_prev = t_now;
      initialize_data_stream(uart_port, MAVLINK_MSG_ID_GLOBAL_POSITION_INT, 100000, m_buffer);
    }

    auto gps_msg_opt = receive_mavlink_gps(uart_port, rx_data);
    if (!gps_msg_opt.has_value()) {
      continue;
    }
    auto gps_msg = gps_msg_opt.value();
    ODID_UAS_Data uas_data;
    odid_initUasData(&uas_data);
    uas_data.LocationValid = 1;
    uas_data.BasicIDValid[0] = 1;
    uas_data.BasicID[0].IDType = ODID_IDTYPE_SERIAL_NUMBER;
    uas_data.BasicID[0].UAType = ODID_UATYPE_HELICOPTER_OR_MULTIROTOR;
    strncpy(uas_data.BasicID[0].UASID, uas_id, ODID_ID_SIZE);
    uas_data.Location.AltitudeGeo = gps_msg.alt;
    uas_data.Location.Latitude = gps_msg.lat;
    uas_data.Location.Longitude = gps_msg.lon;
    int len = odid_wifi_build_message_pack_beacon_frame(&uas_data, (char *)mac, "RID", 3, 0x0064, msg_counter++,
                                                        send_buffer_self, sizeof(send_buffer_self));
    if (len < 0) {
      printf("Error building odid message pack%d\n", len);
      continue;

      ESP_ERROR_CHECK(esp_wifi_80211_tx(WIFI_IF_STA, send_buffer_self, len, false));
    }
    vTaskDelay(1);
  }
}
