#include "interrupt.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static constexpr gpio_num_t INT_GEN_PIN = GPIO_NUM_37;
static constexpr gpio_num_t INT_PIN = GPIO_NUM_36;

static TickType_t int_gen_period = portMAX_DELAY;

void interrupt_gen_task(void *args) {
  uint32_t level = 0;
  for (;;) {
    vTaskDelay(int_gen_period);
    gpio_set_level(INT_GEN_PIN, level);
    level = !level;
  }
}

static void IRAM_ATTR transfer_isr_handler(void *arg) {
  TaskHandle_t acquisition_task = (TaskHandle_t)arg;
  BaseType_t higherPriorityTaskWoken = pdFALSE;
  vTaskNotifyGiveFromISR(acquisition_task, &higherPriorityTaskWoken);
  portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

esp_err_t interrupt_init() {
  // INT input
  gpio_config_t io_conf = {.pin_bit_mask = 1ULL << INT_PIN,
                           .mode = GPIO_MODE_INPUT,
                           .pull_up_en = GPIO_PULLUP_DISABLE,
                           .pull_down_en = GPIO_PULLDOWN_DISABLE,
                           .intr_type = GPIO_INTR_POSEDGE};
  ESP_ERROR_CHECK(gpio_config(&io_conf));

  // INT signal generator
  gpio_config_t int_gen_config = {.pin_bit_mask = 1ULL << INT_GEN_PIN,
                                  .mode = GPIO_MODE_OUTPUT,
                                  .pull_up_en = GPIO_PULLUP_DISABLE,
                                  .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                  .intr_type = GPIO_INTR_DISABLE};
  ESP_ERROR_CHECK(gpio_config(&int_gen_config));

  return ESP_OK;
}

esp_err_t interrupt_start(void *args, int period_ms) {
  ESP_ERROR_CHECK(gpio_install_isr_service(0));
  ESP_ERROR_CHECK(gpio_isr_handler_add(INT_PIN, transfer_isr_handler, args));

  int_gen_period = pdMS_TO_TICKS(period_ms);
  xTaskCreate(interrupt_gen_task, "int-gen", 1024, NULL, 10, NULL);

  return ESP_OK;
}