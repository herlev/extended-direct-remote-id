// Example toggling four LEDs depending on button status
#include "all/mavlink.h"
#include "common/mavlink_msg_adsb_vehicle.h"
#include "driver/uart.h"
#include "mavlink_helpers.h"
#include "mavlink_types.h"
#include <cstring>

void send(uart_port_t uart_num, uint8_t system_id, uint8_t component_id, uint8_t channel, char *callsign, uint32_t icao,
          uint32_t lat, uint32_t lon, uint8_t emitter_type) {
  mavlink_message_t msg;
  uint8_t buf[MAVLINK_MAX_PACKET_LEN] = {};

  int len = mavlink_msg_adsb_vehicle_pack(system_id, component_id, &msg, icao, lat * 10000000, lon * 10000000, 1, 30000,
                                          0, 0, 0, callsign, emitter_type, 1, 0b11111, 6969);
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
int length = 0;
// Configure UART parameters
extern "C" void app_main(void) {
  ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
  ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, 21, 20, 6, 7));

  char cs1[] = "UAV1";
  char cs2[] = "UAV2";
  ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, uart_buffer_size, uart_buffer_size, 100, &uart_queue, 0));
  // Write data to UART.
  while (true) {
    send(uart_num, 1, 156, 5, cs1, 3, 54, 10, ADSB_EMITTER_TYPE_UAV);
    ESP_ERROR_CHECK(uart_wait_tx_done(uart_num, 100));
    send(uart_num, 1, 156, 5, cs2, 2, 55, 10, ADSB_EMITTER_TYPE_GLIDER);
    ESP_ERROR_CHECK(uart_wait_tx_done(uart_num, 100));
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}