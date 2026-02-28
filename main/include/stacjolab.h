#pragma once

#include "sdkconfig.h"
#include "esp_err.h"

#include "pin_def.h"

#include "h_bridge.h"
#include "cc_driver.h"
#include "power_switch.h"
#include "thermocouple.h"
#include "ws28_driver.h"
#include "servo.h"
#include "tensometer.h"

// SERVO
#define LEDC_TIMER_0_FREQ 50
#define LEDC_TIMER_0_RESOLUTION LEDC_TIMER_14_BIT

// HEATER
#define LEDC_TIMER_1_FREQ 100
#define LEDC_TIMER_1_RESOLUTION LEDC_TIMER_14_BIT

// PUMP
#define LEDC_TIMER_2_FREQ 2000
#define LEDC_TIMER_2_RESOLUTION LEDC_TIMER_14_BIT

// UNUSED
#define LEDC_TIMER_3_FREQ 1000
#define LEDC_TIMER_3_RESOLUTION LEDC_TIMER_14_BIT

#define TC_HEATER TC_0_ID
#define TC_INSIDE_OVEN TC_1_ID

#define POWER_SWITCH_HEATER POWER_SWITCH_0_ID

#define MAX_HEATER_TEMP 150
#define STACJOLAB_HIGH_TEMP_THRESHOLD 100
#define STACJOLAB_LOW_TEMP_THRESHOLD 97
#define STACJOLAB_HEATER_DUTY_CYCLE 85

#define TEMP_READ_TASK_INTERVAL_MS 500
#define TEMP_CONTROL_TASK_INTERVAL_MS 1000

#define STACJOLAB_RED_ON_BOOT 255
#define STACJOLAB_GREEN_ON_BOOT 0
#define STACJOLAB_BLUE_ON_BOOT 255

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

    led_strip_t led_strip;

    servo_t servo;

}stacjolab_controller_t;

extern stacjolab_controller_t stacjolab_controller;

esp_err_t stacjolab_controller_init(stacjolab_controller_t* controller);

void temp_read_task(void *arg);
void temp_control_task(void *arg);