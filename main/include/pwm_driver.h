#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "driver/ledc.h"
#include "hal/ledc_types.h"
#include "soc/gpio_num.h"

typedef struct {
    ledc_mode_t speed_mode;
    ledc_timer_t timer;
    ledc_channel_t channel;
    gpio_num_t gpio_num;
    uint32_t freq_hz;
    ledc_timer_bit_t duty_resolution;
} pwm_driver_t;

esp_err_t pwm_driver_init(pwm_driver_t* driver);
esp_err_t pwm_driver_set_duty_raw(const pwm_driver_t* driver, uint32_t duty);
esp_err_t pwm_driver_set_duty_percent(const pwm_driver_t* driver, float percent);