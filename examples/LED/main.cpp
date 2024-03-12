// Example toggling four LEDs depending on button status
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
const gpio_num_t LED1 = GPIO_NUM_21;
const gpio_num_t LED2 = GPIO_NUM_7;
const gpio_num_t LED3 = GPIO_NUM_6;
const gpio_num_t LED4 = GPIO_NUM_5;
const gpio_num_t button = GPIO_NUM_20;
const gpio_config_t pin_conf_LED = {
    .pin_bit_mask = (0b1 << LED1) | (0b1 << LED2) | (0b1 << LED3) | (0b1 << LED4),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
};
const gpio_config_t pin_conf_button = {
    .pin_bit_mask = 0b1 << button,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
};
extern "C" void app_main(void) {

  ESP_ERROR_CHECK(gpio_config(&pin_conf_LED));
  ESP_ERROR_CHECK(gpio_config(&pin_conf_button));
  bool led_status = false;
  while (true) {
    bool led_status = gpio_get_level(button);
    printf("GPIO read: %d\n", led_status);
    ESP_ERROR_CHECK(gpio_set_level(LED1, led_status));
    ESP_ERROR_CHECK(gpio_set_level(LED2, !led_status));
    ESP_ERROR_CHECK(gpio_set_level(LED3, led_status));
    ESP_ERROR_CHECK(gpio_set_level(LED4, !led_status));
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}