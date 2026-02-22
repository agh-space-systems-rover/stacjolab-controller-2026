#pragma once

#include "pwm_driver.h"

#include "esp_err.h"

typedef struct {
    pwm_driver_t pwm_driver;
} cc_driver_t;

esp_err_t cc_driver_init(cc_driver_t *cc_driver);
esp_err_t cc_driver_duty(cc_driver_t *cc_driver, float duty);