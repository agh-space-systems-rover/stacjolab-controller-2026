#pragma once

#include "pwm_driver.h"

#include "esp_err.h"

#define POWER_SWITCH_0_ID 0x0
#define POWER_SWITCH_1_ID 0x1
#define POWER_SWITCH_2_ID 0x2

typedef struct {
    uint8_t id;
    uint8_t enabled;
    float duty;
    pwm_driver_t pwm_driver;
} power_switch_t;

esp_err_t power_switch_init(power_switch_t *power_switch, uint8_t id, ledc_timer_t timer, ledc_channel_t channel, gpio_num_t gpio_num, uint16_t freq_hz, ledc_timer_bit_t duty_resolution);
power_switch_t* get_power_switch_by_id(uint8_t id);
esp_err_t power_switch_set_duty(power_switch_t *power_switch, float duty);
esp_err_t power_switch_enable(power_switch_t *power_switch, uint8_t enabled);