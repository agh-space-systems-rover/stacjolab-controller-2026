#pragma once

#include "led_strip_encoder.h"
#include "driver/rmt_tx.h"

#include "pin_def.h"
#include "config.h"

#define RMT_RESOLUTION_HZ 10000000 // 10MHz, 0.1us per tick
#define LED_STRIP_GPIO_NUM WS2812_PIN

typedef struct {
    rmt_encoder_handle_t encoder;
    rmt_channel_handle_t channel;
    rmt_transmit_config_t tx_config;

    uint8_t led_buffer[LED_STRIP_SIZE * 3];

} led_strip_t;

esp_err_t led_strip_init(led_strip_t *strip);
esp_err_t led_strip_set_solid_color(led_strip_t *strip, uint8_t red, uint8_t green, uint8_t blue);
esp_err_t led_strip_set_single_led(led_strip_t *strip, int index, uint8_t red, uint8_t green, uint8_t blue);