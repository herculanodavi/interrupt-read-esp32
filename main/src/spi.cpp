#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "multi_heap.h"

static constexpr spi_host_device_t HOST = SPI2_HOST;
static constexpr gpio_num_t CS = GPIO_NUM_13;
static constexpr gpio_num_t DO = GPIO_NUM_7;
static constexpr gpio_num_t DI = GPIO_NUM_8;
static constexpr gpio_num_t SCLK = GPIO_NUM_6;

void data_handler_task(void *args) {
  constexpr std::size_t buffer_size = 512 * 7;

  uint32_t caps = MALLOC_CAP_DMA | MALLOC_CAP_32BIT;
  // uint32_t caps = 0;

  uint8_t *tx = (uint8_t *)heap_caps_malloc(buffer_size, caps);
  uint8_t *rx = (uint8_t *)heap_caps_malloc(buffer_size, caps);

  spi_device_handle_t spi = (spi_device_handle_t)args;
  spi_transaction_t trans = {.flags = 0,
                             .cmd = 0,
                             .addr = 0,
                             .length = buffer_size * 8,
                             .rxlength = 0,
                             .user = nullptr,
                             .tx_buffer = tx,
                             .rx_buffer = rx};

  ESP_ERROR_CHECK(spi_device_acquire_bus(spi, portMAX_DELAY));

  for (;;) {
    ulTaskNotifyTake(true, portMAX_DELAY);
    ESP_ERROR_CHECK(spi_device_transmit(spi, &trans));
  }
}

spi_device_handle_t spi_init() {
  spi_device_handle_t spi;

  spi_bus_config_t buscfg = {.mosi_io_num = DO,
                             .miso_io_num = DI,
                             .sclk_io_num = SCLK,
                             .quadwp_io_num = -1,
                             .quadhd_io_num = -1,
                             .data4_io_num = -1,
                             .data5_io_num = -1,
                             .data6_io_num = -1,
                             .data7_io_num = -1,
                             .max_transfer_sz = 0,
                             .flags = 0,
                             .isr_cpu_id = INTR_CPU_ID_AUTO,
                             .intr_flags = 0};

  spi_device_interface_config_t devcfg = {.command_bits = 0,
                                          .address_bits = 0,
                                          .dummy_bits = 0,
                                          .mode = 0,
                                          .clock_source = SPI_CLK_SRC_DEFAULT,
                                          .duty_cycle_pos = 0,
                                          .cs_ena_pretrans = 0,
                                          .cs_ena_posttrans = 0,
                                          .clock_speed_hz = 10'000'000,
                                          .input_delay_ns = 0,
                                          .spics_io_num = CS,
                                          .flags = 0,
                                          .queue_size = 7,
                                          .pre_cb = nullptr,
                                          .post_cb = nullptr};

  ESP_ERROR_CHECK(spi_bus_initialize(HOST, &buscfg, SPI_DMA_CH_AUTO));
  ESP_ERROR_CHECK(spi_bus_add_device(HOST, &devcfg, &spi));

  return spi;
}

TaskHandle_t spi_handler_start(void *args) {
  TaskHandle_t task = nullptr;
  xTaskCreate(data_handler_task, "data-handler", 16000, args, 10, &task);
  return task;
}