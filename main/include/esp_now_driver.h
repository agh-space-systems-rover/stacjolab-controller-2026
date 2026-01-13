#pragma once

#include "freertos/idf_additions.h"
#include <stddef.h>

extern QueueHandle_t q_esp_now_rx;
extern QueueHandle_t q_esp_now_tx;
extern QueueHandle_t q_esp_now_rx_raw;


void esp_now_register_callbacks();
void esp_now_task(void *arg);