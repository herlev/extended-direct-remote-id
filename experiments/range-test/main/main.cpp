#include "freertos/FreeRTOS.h"
#include <stdio.h>

extern "C" void app_main(void) {
  while (true) {
    printf("Range test\n");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
