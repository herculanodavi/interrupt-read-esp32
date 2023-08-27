#pragma once

#include "driver/gpio.h"
#include "esp_err.h"

esp_err_t interrupt_init();

esp_err_t interrupt_start(void *args, int period_ms);