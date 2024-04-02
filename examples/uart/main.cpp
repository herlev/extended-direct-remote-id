#include "driver/uart.h"
#include <cstring>
#include "esp_efuse.h"

const uart_port_t uart_num = UART_NUM_1;
uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = 122,
};
const int uart_buffer_size = (1024 * 2);
QueueHandle_t uart_queue;
char *test_str = "This is a test string.\n";
uint8_t rx_data[128];
int length = 0;
// Configure UART parameters
extern "C" void app_main(void) {
  esp_efuse_set_rom_log_scheme(ESP_EFUSE_ROM_LOG_ON_GPIO_HIGH);
  ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
  ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, 21, 20, 6, 7));
  // Setup UART buffered IO with event queue
  // Install UART driver using an event queue here
  ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, uart_buffer_size, uart_buffer_size, 100, &uart_queue, 0));
  // Write data to UART.
  while (true) {
    printf("sending uart msg\n");
    uart_write_bytes(uart_num, (const char *)test_str, strlen(test_str));
    // Wait for packet to be sent
    ESP_ERROR_CHECK(uart_wait_tx_done(uart_num, 100)); // wait timeout is 100 RTOS ticks (TickType_t)
    length = 0;
    ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_num, (size_t *)&length));
    length = uart_read_bytes(uart_num, rx_data, length, 100);
    if(length){
      printf("Received on uart: ");
      for (int i = 0; i < length; i++)
      {
        printf("%c",rx_data[i]);
      }
      
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}