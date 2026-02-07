#include "stacjolab.h"

#include "esp_log.h"
#include "esp_err.h"

esp_err_t stacjolab_controller_init(stacjolab_controller_t* controller) {
    if(controller == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize debug led
        gpio_config_t io_conf = {
            .intr_type = GPIO_INTR_DISABLE,
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = (1ULL << LED1_PIN) | (1ULL << LED2_PIN),
            .pull_down_en = 0,
            .pull_up_en = 0
        };
    gpio_config(&io_conf);

    // Initialize H-Bridge channels
    ESP_ERROR_CHECK(h_bridge_init(&controller->h_bridge_ch_1, H_BRIDGE_1_ID));
    ESP_ERROR_CHECK(h_bridge_init(&controller->h_bridge_ch_0, H_BRIDGE_0_ID));

    // Initialize CC driver
    ESP_ERROR_CHECK(cc_driver_init(&controller->cc_driver));

    // Initialize power switches
    ESP_ERROR_CHECK(power_switch_init(&controller->power_switch_ch_0, POWER_SWITCH_0_ID));
    ESP_ERROR_CHECK(power_switch_init(&controller->power_switch_ch_1, POWER_SWITCH_1_ID));
    ESP_ERROR_CHECK(power_switch_init(&controller->power_switch_ch_2, POWER_SWITCH_2_ID));

    ESP_LOGI("stacjolab", "Controller initialized");

    return ESP_OK;
}