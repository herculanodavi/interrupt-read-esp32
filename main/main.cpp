#include <esp_log.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "interrupt.hpp"
#include "spi.hpp"
#include "stats.hpp"

extern "C" void app_main(void) {
  // Init peripherals
  interrupt_init();
  spi_device_handle_t spi = spi_init();
  TaskHandle_t handler_task = spi_handler_start((void *)spi);

  // Start interrupt generator task
  int period_ms = 10;
  interrupt_start((void *)handler_task, period_ms);

  // Start freertos stats task
  stats_init();

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
