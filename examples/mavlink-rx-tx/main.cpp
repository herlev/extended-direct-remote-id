#include "all/mavlink.h"
#include "common/mavlink_msg_attitude.h"
#include "common/mavlink_msg_command_long.h"
#include "driver/uart.h"
#include "custom-efuse.hpp"
#include "mavlink_helpers.h"
void request_message_interval(uart_port_t uart_num, uint8_t system_id, uint8_t component_id, uint16_t message_id,
                              int32_t interval_us, uint8_t confirmation) {
  mavlink_message_t msg;
  uint8_t buf[MAVLINK_MAX_PACKET_LEN] = {};
  mavlink_msg_command_long_pack(system_id, component_id, &msg, system_id, 0, MAV_CMD_SET_MESSAGE_INTERVAL, confirmation,
                                message_id, interval_us, 0, 0, 0, 0, 0);
  int l = mavlink_msg_to_send_buffer(buf, &msg);
  uart_write_bytes(uart_num, buf, l);
}
const uart_port_t uart_num = UART_NUM_1;
uart_config_t uart_config = {
    .baud_rate = 57600,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = 122,
};
const int uart_buffer_size = (1024 * 2);
QueueHandle_t uart_queue;
uint8_t rx_data[128];
int length = 0;
mavlink_message_t msg;
mavlink_status_t status;
mavlink_attitude_t attitude;

extern "C" void app_main(void) {
  disable_UART_boot_logging();
  ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
  ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, 21, 20, 6, 7));
  ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, uart_buffer_size, uart_buffer_size, 100, &uart_queue, 0));

  uint8_t system_id = 1;
  uint8_t component_id = 156;
  uint16_t message_id = 30;
  uint32_t interval_us = 100000;
  vTaskDelay(500 / portTICK_PERIOD_MS);
  request_message_interval(uart_num, system_id, component_id, message_id, interval_us, 0);

  ESP_ERROR_CHECK(uart_wait_tx_done(uart_num, 100));
  while (true) {
    ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_num, (size_t *)&length));
    length = uart_read_bytes(uart_num, rx_data, length, 100);

    if (length) {
      for (int i = 0; i < length; i++) {
        uint8_t byte = rx_data[i];
        if (mavlink_parse_char(0, byte, &msg, &status)) {
          // printf("Received message with ID %d, sequence: %d from component %d of system %d\n", msg.msgid, msg.seq,
          //        msg.compid, msg.sysid);
          if (msg.msgid == 30) {
            mavlink_msg_attitude_decode(&msg, &attitude);
            printf("Roll: %f Pitch: %f, Yaw: %f\n", attitude.roll, attitude.pitch, attitude.yaw);
          }
        }
      }
    }

    vTaskDelay(1);
  }
}