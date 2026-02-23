
#include "ws28_driver.h"

#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "ws28_driver";

esp_err_t led_strip_init(led_strip_t *strip){
    ESP_LOGI(TAG, "Create RMT TX channel");
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = LED_STRIP_GPIO_NUM,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz = RMT_RESOLUTION_HZ,
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &strip->channel));

    ESP_LOGI(TAG, "Create LED strip encoder");
    led_strip_encoder_config_t encoder_config = {
        .resolution = RMT_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &strip->encoder));

    ESP_ERROR_CHECK(rmt_enable(strip->channel));

    strip->tx_config.loop_count = 0; // no loop

    return ESP_OK;
}

esp_err_t led_strip_set_solid_color(led_strip_t *strip, uint8_t red, uint8_t green, uint8_t blue){
    for (int i = 0; i < LED_STRIP_SIZE; i++) {
        strip->led_buffer[i * 3 + 0] = green;
        strip->led_buffer[i * 3 + 1] = red;
        strip->led_buffer[i * 3 + 2] = blue;
    }

    return rmt_transmit(strip->channel, strip->encoder, strip->led_buffer, sizeof(strip->led_buffer), &strip->tx_config);
}

esp_err_t led_strip_set_single_led(led_strip_t *strip, int index, uint8_t red, uint8_t green, uint8_t blue){
    if (index < 0 || index >= LED_STRIP_SIZE) {
        return ESP_ERR_INVALID_ARG;
    }

    strip->led_buffer[index * 3 + 0] = green;
    strip->led_buffer[index * 3 + 1] = red;
    strip->led_buffer[index * 3 + 2] = blue;

    return rmt_transmit(strip->channel, strip->encoder, strip->led_buffer, sizeof(strip->led_buffer), &strip->tx_config);
}