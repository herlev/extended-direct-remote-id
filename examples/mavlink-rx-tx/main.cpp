#include "esp_timer.h"
#include "mavlink-uart.hpp"
#define UART_TX GPIO_NUM_21
#define UART_RX GPIO_NUM_20
uint8_t rx_data[MAVLINK_MAX_PACKET_LEN] = {};
uint8_t m_buffer[MAVLINK_MAX_PACKET_LEN] = {};

extern "C" void app_main(void) {

  uart_port_t uart_port = UART_NUM_1;
  initialize_uart(uart_port, UART_TX, UART_RX, 2000000);
  uint32_t t_prev = (uint32_t)esp_timer_get_time() / 1000;
  uint32_t t_now = t_prev;
  while (true) {
    t_now = (uint32_t)esp_timer_get_time() / 1000;
    if (t_now - t_prev > 10000) {
      t_prev = t_now;
      printf("Sending stream request\n");
      initialize_data_stream(uart_port, MAVLINK_MSG_ID_ATTITUDE, 1000000, m_buffer);
      initialize_data_stream(uart_port, MAVLINK_MSG_ID_GLOBAL_POSITION_INT, 100000, m_buffer);
    }
    auto gps_msg_opt = receive_mavlink_gps(uart_port, rx_data);
    if (gps_msg_opt.has_value()) {
      auto gps_msg = gps_msg_opt.value();
      printf("GPS values. Alt: %lu, Lat: %lu, Lon: %lu\n", gps_msg.alt, gps_msg.lat, gps_msg.lon);
    }
    vTaskDelay(1);
  }
}
