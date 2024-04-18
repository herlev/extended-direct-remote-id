#include "all/mavlink.h"
#include "common/mavlink_msg_attitude.h"
#include "common/mavlink_msg_command_long.h"
#include "common/mavlink_msg_global_position_int.h"
#include "custom-efuse.hpp"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "mavlink_helpers.h"
#include <optional>

#define UART_TX GPIO_NUM_21
#define UART_RX GPIO_NUM_20
uint8_t rx_data[MAVLINK_MAX_PACKET_LEN] = {};
void initialize_uart(uart_port_t uart_num = UART_NUM_1, int uart_tx_pin = GPIO_NUM_21, int uart_rx_pin = GPIO_NUM_20) {
  uart_config_t uart_config = {
      .baud_rate = 2000000,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .rx_flow_ctrl_thresh = 122,
  };
  const int uart_buffer_size = (1024 * 2);
  ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
  ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, uart_tx_pin, uart_rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
  ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, uart_buffer_size, uart_buffer_size, 0, NULL, 0));
}

std::optional<mavlink_global_position_int_t> receive_mavlink_gps(uart_port_t uart_port) {
  mavlink_global_position_int_t mavlink_gps_msg;
  size_t length = 0;
  mavlink_message_t msg;
  mavlink_attitude_t attitude;
  mavlink_status_t status;
  ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_port, &length));
  length = uart_read_bytes(uart_port, (void *)rx_data, length, 100);

  if (length) {
    for (int i = 0; i < length; i++) {
      uint8_t byte = rx_data[i];
      if (mavlink_parse_char(0, byte, &msg, &status)) {
        if (msg.msgid == MAVLINK_MSG_ID_GLOBAL_POSITION_INT) {
          mavlink_msg_global_position_int_decode(&msg, &mavlink_gps_msg);
          return mavlink_gps_msg;
        }
        if (msg.msgid == MAVLINK_MSG_ID_ATTITUDE) {
          mavlink_msg_attitude_decode(&msg, &attitude);
          printf("Roll: %f Pitch: %f, Yaw: %f\n", attitude.roll, attitude.pitch, attitude.yaw);
        }
      }
    }
  }
  return {};
}
void initialize_data_stream(uart_port_t uart_port, uint16_t message_id, uint32_t message_inteval_us) {
  vTaskDelay(500 / portTICK_PERIOD_MS); // Delay because of boot UART messages
  uint8_t system_id = 1;
  uint8_t component_id = 156;
  mavlink_message_t msg;
  uint8_t buf[MAVLINK_MAX_PACKET_LEN] = {};
  mavlink_msg_command_long_pack(system_id, component_id, &msg, system_id, 0, MAV_CMD_SET_MESSAGE_INTERVAL, 0,
                                message_id, message_inteval_us, 0, 0, 0, 0, 0);
  int l = mavlink_msg_to_send_buffer(buf, &msg);
  uart_write_bytes(uart_port, buf, l);
  ESP_ERROR_CHECK(uart_wait_tx_done(uart_port, 100));
  return;
}
void request_message_interval(uart_port_t uart_num, uint8_t system_id, uint8_t component_id, uint16_t message_id,
                              int32_t interval_us, uint8_t confirmation) {
  mavlink_message_t msg;
  uint8_t buf[MAVLINK_MAX_PACKET_LEN] = {};
  mavlink_msg_command_long_pack(system_id, component_id, &msg, system_id, 0, MAV_CMD_SET_MESSAGE_INTERVAL, confirmation,
                                message_id, interval_us, 0, 0, 0, 0, 0);
  int l = mavlink_msg_to_send_buffer(buf, &msg);
  uart_write_bytes(uart_num, buf, l);
}
extern "C" void app_main(void) {

  uart_port_t uart_port = UART_NUM_1;
  initialize_uart(uart_port);
  initialize_data_stream(uart_port, MAVLINK_MSG_ID_ATTITUDE, 1000000);
  initialize_data_stream(uart_port, MAVLINK_MSG_ID_GLOBAL_POSITION_INT, 100000);
  while (true) {
    auto gps_msg_opt = receive_mavlink_gps(uart_port);
    if (gps_msg_opt.has_value()) {
      auto gps_msg = gps_msg_opt.value();
      printf("GPS values. Alt: %lu, Lat: %lu, Lon: %lu\n", gps_msg.alt, gps_msg.lat, gps_msg.lon);
    }
    vTaskDelay(1);
  }
}
