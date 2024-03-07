
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
const gpio_num_t pin = GPIO_NUM_10;
const gpio_config_t pin_conf = {
    .pin_bit_mask = 0b1 << pin,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
};
extern "C" void app_main(void) {

  ESP_ERROR_CHECK(gpio_config(&pin_conf));

  while (true) {
    printf("Pin level: %u \n", gpio_get_level(pin));
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}