#include <esp_log.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static spi_host_device_t HOST = SPI2_HOST;

static gpio_num_t CS = 13;
static gpio_num_t DO = 7;
static gpio_num_t DI = 8;
static gpio_num_t SCLK = 6;

static gpio_num_t INT_GEN_PIN = 37;
static gpio_num_t INT_PIN = 36;

typedef struct transaction_args {
  spi_device_handle_t spi;
  spi_transaction_t *trans;
} transaction_args_t;

void interrupt_gen_task(void *args) {
  uint32_t level = 0;
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(INT_GEN_PIN, level);
    level = !level;
  }
}

static void IRAM_ATTR gpio_isr_handler(void *arg) {
  transaction_args_t *trans_args = (transaction_args_t *)arg;
  ESP_ERROR_CHECK(
      spi_device_queue_trans(trans_args->spi, trans_args->trans, 0));
}

void data_handler_task(void *args) {
  transaction_args_t *trans_args = (transaction_args_t *)args;
  spi_transaction_t *received_trans_args = NULL;

  for (;;) {
    spi_device_get_trans_result(trans_args->spi, &received_trans_args,
                                portMAX_DELAY);
  }
}

void app_main(void) {
  // Setup peripherals
  // SPI
  spi_device_handle_t spi;

  spi_bus_config_t buscfg = {.miso_io_num = DI,
                             .mosi_io_num = DO,
                             .sclk_io_num = SCLK,
                             .quadwp_io_num = -1,
                             .quadhd_io_num = -1};

  spi_device_interface_config_t devcfg = {
      .clock_speed_hz = 10 * 1000 * 1000,
      .mode = 0,
      .spics_io_num = CS,
      .queue_size = 7,
  };

  ESP_ERROR_CHECK(spi_bus_initialize(HOST, &buscfg, SPI_DMA_CH_AUTO));
  ESP_ERROR_CHECK(spi_bus_add_device(HOST, &devcfg, &spi));

  // INT input
  gpio_config_t io_conf = {.pin_bit_mask = 1ULL << INT_PIN,
                           .mode = GPIO_MODE_INPUT,
                           .pull_up_en = 0,
                           .pull_down_en = 1,
                           .intr_type = GPIO_INTR_POSEDGE};
  ESP_ERROR_CHECK(gpio_config(&io_conf));

  // INT signal generator
  gpio_config_t int_gen_config = {.pin_bit_mask = 1ULL << INT_GEN_PIN,
                                  .mode = GPIO_MODE_OUTPUT,
                                  .pull_up_en = 0,
                                  .pull_down_en = 0,
                                  .intr_type = GPIO_INTR_DISABLE};
  gpio_config(&int_gen_config);

  // Create SPI transaction arguments
  spi_transaction_t trans;
  memset(&trans, 0, sizeof(spi_transaction_t));
  trans.tx_data[0] = 0xff;
  trans.tx_data[2] = 0xff;
  trans.length = 4;
  transaction_args_t args = {.spi = spi, .trans = &trans};

  // Configure interrupt service
  ESP_ERROR_CHECK(gpio_install_isr_service(0));
  ESP_ERROR_CHECK(
      gpio_isr_handler_add(INT_PIN, gpio_isr_handler, (void *)&args));

  // Start data handler task
  xTaskCreate(data_handler_task, "data-handler", 1024, NULL, 10, (void *)&args);

  // Start interrupt generator task
  xTaskCreate(interrupt_gen_task, "int-gen", 1024, NULL, 10, NULL);

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
