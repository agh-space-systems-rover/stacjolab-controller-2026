#pragma once
#include "pwm_driver.h"

#define SERVO_0_MAX_ANGLE 100
#define SERVO_0_MAX_PULSE_WIDTH_US 2100
#define SERVO_0_MIN_PULSE_WIDTH_US 900

typedef struct {
    pwm_driver_t pwm;
} servo_t;

esp_err_t servo_init(servo_t* servo, gpio_num_t pin, ledc_timer_t timer, ledc_channel_t channel, uint16_t freq_hz, ledc_timer_bit_t duty_resolution);
esp_err_t servo_set_pulse_width(servo_t* servo, uint32_t pulse_width_us);
esp_err_t servo_disable(servo_t* servo);
esp_err_t servo_set_percent(servo_t* servo, uint8_t percent);