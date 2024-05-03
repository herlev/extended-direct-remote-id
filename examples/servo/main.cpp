#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include <stdio.h>
// Example to rotate servo from 0 to 180 deg in increment. Servo connected on GPIO_NUM_8 (D8)
const uint16_t duty_5 = 409;  // pow(2, 13)*0.05
const uint16_t duty_10 = 819; // pow(2, 13)*0.1
float map(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
static void example_ledc_init(void) {
  // Prepare and then apply the LEDC PWM timer configuration
  ledc_timer_config_t ledc_timer = {.speed_mode = LEDC_LOW_SPEED_MODE,
                                    .duty_resolution = LEDC_TIMER_13_BIT,
                                    .timer_num = LEDC_TIMER_0,
                                    .freq_hz = 50, // Set output frequency at 4 kHz
                                    .clk_cfg = LEDC_AUTO_CLK};
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  // Prepare and then apply the LEDC PWM channel configuration
  ledc_channel_config_t ledc_channel = {.gpio_num = GPIO_NUM_8,
                                        .speed_mode = LEDC_LOW_SPEED_MODE,
                                        .channel = LEDC_CHANNEL_0,
                                        .intr_type = LEDC_INTR_DISABLE,
                                        .timer_sel = LEDC_TIMER_0,
                                        .duty = 0,
                                        .hpoint = 0};
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}
uint32_t calculate_duty(float duty) {

  return map(duty, 5, 10, duty_5, duty_10);
}

float angle_to_duty(uint8_t angle_deg) {
  return map(float(angle_deg), 0.0, 180.0, 5.0, 10.0);
}
extern "C" void app_main(void) {
  while (true) {
    example_ledc_init();

    float duty = 0.0;
    uint32_t timer_val = 0;
    for (uint8_t i = 0; i <= 180; i+=10) {
      duty = angle_to_duty(i);
      timer_val = calculate_duty(duty);
      printf("Angle: %d, Dutycycle %.6f, timer_val %lu \n", i, duty, timer_val);
      ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, timer_val));
      ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
      vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}