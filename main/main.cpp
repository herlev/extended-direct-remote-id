#include <stdio.h>
#include "freertos/FreeRTOS.h"

class A {
public:
  void print() { printf("hello c++\n"); }
};

extern "C" void app_main(void) {
  A a;

  while (true) {
    a.print();
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}
