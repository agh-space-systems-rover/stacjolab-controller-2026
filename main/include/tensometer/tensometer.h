#pragma once

#include <stdint.h>
#include "esp_err.h"

void tensometer_init(int sda_pin, int scl_pin);
int32_t tensometer_get_voltage_1(void);
int32_t tensometer_get_voltage_2(void);
int32_t tensometer_get_voltage_sum(void);
void tensometer_tare(void);
