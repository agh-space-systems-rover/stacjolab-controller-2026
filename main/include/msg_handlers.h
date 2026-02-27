#pragma once
#include "freertos/idf_additions.h"
#include "tlv.h"

extern QueueHandle_t q_esp_now_tx;

void on_ping(const msg_t *msg, void *user_ctx);

#define PS_SET_MSG_TYPE 0x10
typedef struct {
    uint8_t channel;         // PWM channel ID
    uint8_t duty_cycle;      // Duty cycle from 0 to 100
} ps_set_msg_t;
void on_ps_set(const msg_t *msg, void *user_ctx);

#define HEATER_MSG_TYPE 0x11
typedef struct {
    uint8_t enabled;        // 0 = off, 1 = on
} heater_msg_t;
void on_heater_enable(const msg_t *msg, void *user_ctx);

#define HEATER_CONFIG_MSG_TYPE 0x12
typedef struct {
    uint8_t high_temp_threshold;
    uint8_t low_temp_threshold;
    uint8_t heater_duty_cycle;
} heater_config_msg_t;
void on_heater_config(const msg_t *msg, void *user_ctx);

#define GET_TC_TEMP_MSG_TYPE 0x15
typedef struct {
    uint8_t tc_id;
} get_tc_temp_msg_t;
void on_get_tc_temp(const msg_t *msg, void *user_ctx);

#define RESP_TC_TEMP_MSG_TYPE 0x16
typedef struct {
    uint8_t tc_id;
    int16_t temp;   // 21.37 C = 2137
    // add flags for fault conditions (open circuit, short to GND, short to VCC) as bit fields
    uint8_t fault;  // 0 = no fault, 1 = fault detected
} resp_tc_temp_msg_t;

#define H_BRIDGE_MSG_TYPE 0x50
typedef struct {
    int8_t h_bridge_id;
    int8_t speed;           // Speed from -100 to 100
} h_bridge_msg_t;
void on_h_bridge_set_speed(const msg_t *msg, void *user_ctx);

#define CC_DRIVER_MSG_TYPE 0x51
typedef struct {
    uint8_t duty;            // Duty cycle from 0 to 100
} cc_driver_msg_t;
void on_cc_driver_set_duty(const msg_t *msg, void *user_ctx);

