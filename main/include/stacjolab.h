#pragma once

#include "sdkconfig.h"
#include "esp_err.h"

#include "pin_def.h"

#include "h_bridge.h"
#include "cc_driver.h"
#include "power_switch.h"
#include "thermocouple.h"

#define LEDC_TIMER_0_FREQ 100
#define LEDC_TIMER_0_RESOLUTION LEDC_TIMER_14_BIT

#define TC_HEATER TC_0_ID
#define TC_INSIDE_OVEN TC_1_ID

#define POWER_SWITCH_HEATER POWER_SWITCH_0_ID

#define MAX_HEATER_TEMP CONFIG_STACJOLAB_MAX_HEATER_TEMP

#define TEMP_READ_TASK_INTERVAL_MS 500
#define TEMP_CONTROL_TASK_INTERVAL_MS 1000

typedef struct {

    float high_temp_threshold;
    float low_temp_threshold;

    float heater_duty_cycle;

    bool heating_enabled;

}temp_control_config_t;

typedef struct {
    
    h_bridge_t h_bridge_ch_0;
    h_bridge_t h_bridge_ch_1;
    
    cc_driver_t cc_driver;
    
    power_switch_t power_switch_ch_0;
    power_switch_t power_switch_ch_1;
    power_switch_t power_switch_ch_2;
    
    thermocouple_manager_t thermocouple_manager;
    
    temp_control_config_t temp_control_config;
    
}stacjolab_controller_t;

extern stacjolab_controller_t stacjolab_controller;

esp_err_t stacjolab_controller_init(stacjolab_controller_t* controller);

void temp_read_task(void *arg);
void temp_control_task(void *arg);