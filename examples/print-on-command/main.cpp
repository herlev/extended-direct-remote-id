
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"

void print_results(uint32_t res[], uint32_t len) {
  printf("-----------------Results------------ \n");
  for (int i = 0; i < len; i++) {
    printf("%lu\n", res[i]);
  }
}
const uint32_t len = 10000;
static uint32_t table[len] = {};
extern "C" void app_main(void) {
  uint32_t i = 0;
  while (true) {
    table[i++] = (uint32_t)(esp_timer_get_time() / 1000);
    int c = getc(stdin);

    if (c == 'p') {
      print_results(table, len);
    }
    if (i == len) {
      i = 0;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}