#pragma once
#include "freertos/idf_additions.h"
#include "tlv.h"

extern QueueHandle_t q_esp_now_tx;

void on_ping(const msg_t *msg, void *user_ctx);

#define H_BRIDGE_MSG_TYPE 0x10
typedef struct {
    int8_t h_bridge_id;
    int8_t speed;           // Speed from -100 to 100
} h_bridge_msg_t;
void on_h_bridge_set_speed(const msg_t *msg, void *user_ctx);

#define HEATER_MSG_TYPE 0x11
typedef struct {
    uint8_t enabled;        // 0 = off, 1 = on
} heater_msg_t;
void on_heater(const msg_t *msg, void *user_ctx);