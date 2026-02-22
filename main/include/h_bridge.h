#pragma once

#include <pwm_driver.h>

#include "esp_err.h"

#define H_BRIDGE_0_ID 0x0
#define H_BRIDGE_1_ID 0x1

typedef struct {
    int8_t h_bridge_id;
    pwm_driver_t pwm_driver1;
    pwm_driver_t pwm_driver2;
} h_bridge_t;

esp_err_t h_bridge_init(h_bridge_t* h_bridge, uint8_t h_bridge_id);
h_bridge_t* get_h_bridge_by_id(uint8_t h_bridge_id);
esp_err_t h_bridge_set_speed(h_bridge_t* h_bridge, int8_t speed);