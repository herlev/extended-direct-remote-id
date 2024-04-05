#pragma once
#include "esp_efuse.h"
#include "esp_efuse_table.h"
void disable_UART_boot_logging() {
  const uint8_t uart_disabled_bitfield_value = 0b11;
  uint8_t bitfield_value;
  esp_efuse_read_field_blob(ESP_EFUSE_UART_PRINT_CONTROL, &bitfield_value, 2);
  if (bitfield_value != uart_disabled_bitfield_value) {
    printf("Disabling UART boot logging on this chip permanently\n");
    bitfield_value = uart_disabled_bitfield_value;
    esp_efuse_write_field_blob(ESP_EFUSE_UART_PRINT_CONTROL, &bitfield_value, 2);
  }
}