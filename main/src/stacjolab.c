#include "stacjolab.h"

esp_err_t stacjolab_controller_init(stacjolab_controller_t* controller) {
    if(controller == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize H-Bridge channels
    ESP_ERROR_CHECK(h_bridge_init(&controller->h_bridge_ch_0, H_BRIDGE_0_ID));
    ESP_ERROR_CHECK(h_bridge_init(&controller->h_bridge_ch_1, H_BRIDGE_1_ID));

    return ESP_OK;
}