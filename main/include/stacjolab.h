#pragma once

#include "esp_err.h"

#include "pin_def.h"

#include "h_bridge.h"
#include "cc_driver.h"
#include "power_switch.h"

#define LEDC_TIMER_0_FREQ 100


typedef struct {
    
    h_bridge_t h_bridge_ch_0;
    h_bridge_t h_bridge_ch_1;
    
    cc_driver_t cc_driver;
    
    power_switch_t power_switch_ch_0;
    power_switch_t power_switch_ch_1;
    power_switch_t power_switch_ch_2;
    
}stacjolab_controller_t;

extern stacjolab_controller_t stacjolab_controller;

esp_err_t stacjolab_controller_init(stacjolab_controller_t* controller);