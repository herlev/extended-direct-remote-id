#include "all/mavlink.h"
#include "common/mavlink_msg_command_long.h"
#include "common/mavlink_msg_global_position_int.h"
#include "driver/gpio.h"
#include "driver/uart.h"
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

std::optional<mavlink_global_position_int_t> receive_mavlink_gps(uart_port_t uart_port, uint8_t *rx_data_buffer) {
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
void initialize_data_stream(uart_port_t uart_port, uint16_t message_id, uint32_t message_inteval_us,
                            uint8_t *mavlink_buffer) {
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

void update_dri_mavlink(uart_port_t uart_port, uint8_t *uart_rx_buffer, uint8_t *wifi_tx_buffer, char *uas_id,
                        char *mac, uint8_t msg_counter) {
  auto gps_msg_opt = receive_mavlink_gps(uart_port, uart_rx_buffer);
  if (!gps_msg_opt.has_value()) {
    return;
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
  int len = odid_wifi_build_message_pack_beacon_frame(&uas_data, (char *)mac, "RID", 3, 0x0064, msg_counter,
                                                      wifi_tx_buffer, sizeof(wifi_tx_buffer));
  if (len < 0) {
    printf("Error building odid message pack%d\n", len);
    return;
    ESP_ERROR_CHECK(esp_wifi_80211_tx(WIFI_IF_STA, wifi_tx_buffer, len, false));
  }
}