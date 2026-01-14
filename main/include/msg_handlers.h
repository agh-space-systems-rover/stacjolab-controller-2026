#pragma once
#include "freertos/idf_additions.h"
#include "tlv.h"

extern QueueHandle_t q_esp_now_tx;

void on_ping(const msg_t *msg, void *user_ctx);
