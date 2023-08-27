#pragma once

#include "driver/spi_master.h"

spi_device_handle_t spi_init();

TaskHandle_t spi_handler_start(void *args);