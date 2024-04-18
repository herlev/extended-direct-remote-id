#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "portmacro.h"
#include "routing.hpp"
#include "utils.hpp"
#include <sys/_stdint.h>

#include "dri-scanner.hpp"

extern "C" void app_main(void) {
  initialize();

  scan_callback = [](ScanResult scan_result) {
    printf("count: %u\n", scan_result.odid_msg.msg_counter);
  };

  while (true) {
    printTable();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
