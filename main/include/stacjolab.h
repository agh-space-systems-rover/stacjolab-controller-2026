#pragma once

#include "esp_err.h"

#include "h_bridge.h"

#define LEDC_TIMER_0_FREQ 100

typedef struct {
    h_bridge_t h_bridge_ch_0;
    h_bridge_t h_bridge_ch_1;
}stacjolab_controller_t;

esp_err_t stacjolab_controller_init(stacjolab_controller_t* controller);