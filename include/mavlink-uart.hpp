#include "all/mavlink.h"
#include "common/mavlink_msg_command_long.h"
#include "common/mavlink_msg_global_position_int.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include <optional>
void initialize_uart(uart_port_t uart_num, int uart_tx_pin, int uart_rx_pin, int baud) {
  uart_config_t uart_config = {
      .baud_rate = baud,
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

std::optional<mavlink_global_position_int_t> receive_mavlink_gps(uart_port_t uart_port,uint8_t* rx_data_buffer) {
  mavlink_global_position_int_t mavlink_gps_msg;
  size_t length = 0;
  mavlink_message_t msg;
  mavlink_attitude_t attitude;
  mavlink_status_t status;
  ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_port, &length));
  length = uart_read_bytes(uart_port, (void *)rx_data_buffer, length, 100);
  if (length) {
    for (int i = 0; i < length; i++) {
      uint8_t byte = rx_data_buffer[i];
      if (mavlink_parse_char(0, byte, &msg, &status)) {
        if (msg.msgid == MAVLINK_MSG_ID_GLOBAL_POSITION_INT) {
          mavlink_msg_global_position_int_decode(&msg, &mavlink_gps_msg);
          return mavlink_gps_msg;
        }
        // Useful for debugging when indoors and no GPS fix 
        // if (msg.msgid == MAVLINK_MSG_ID_ATTITUDE) { 
        //   mavlink_msg_attitude_decode(&msg, &attitude);
        //   printf("Roll: %f Pitch: %f, Yaw: %f\n", attitude.roll, attitude.pitch, attitude.yaw);
        // }
      }
    }
  }
  return {};
}
void initialize_data_stream(uart_port_t uart_port, uint16_t message_id, uint32_t message_inteval_us,uint8_t* mavlink_buffer) {
  vTaskDelay(500 / portTICK_PERIOD_MS); // Delay because of boot UART messages
  uint8_t system_id = 1;
  uint8_t component_id = 156;
  mavlink_message_t msg;
  mavlink_msg_command_long_pack(system_id, component_id, &msg, system_id, 0, MAV_CMD_SET_MESSAGE_INTERVAL, 0,
                                message_id, message_inteval_us, 0, 0, 0, 0, 0);
  int l = mavlink_msg_to_send_buffer(mavlink_buffer, &msg);
  uart_write_bytes(uart_port, mavlink_buffer, l);
  ESP_ERROR_CHECK(uart_wait_tx_done(uart_port, 100));
  return;
}