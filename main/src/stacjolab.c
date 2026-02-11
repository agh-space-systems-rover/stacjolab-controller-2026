#include "stacjolab.h"

#include "esp_log.h"
#include "esp_err.h"

static const char* TAG = "stacjolab_controller";

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

    ESP_ERROR_CHECK(thermocouple_manager_init(&controller->thermocouple_manager));

    // Initialize temperature control config
    controller->temp_control_config.high_temp_threshold = 95.0f;
    controller->temp_control_config.low_temp_threshold = 60.0f;
    controller->temp_control_config.heater_duty_cycle = 30.0f;
    controller->temp_control_config.heating_enabled = false;

    ESP_LOGI(TAG, "Controller initialized");

    return ESP_OK;
}

void temp_control_task(void *arg) {

    while (1) {
        ESP_ERROR_CHECK(thermocouple_read_all(&stacjolab_controller.thermocouple_manager));

        ESP_LOGI(TAG, "TC0: %.2f C, fault: %d", get_temperature_by_id(TC_0_ID), get_thermocouple_fault_by_id(TC_0_ID));
        ESP_LOGI(TAG, "TC1: %.2f C, fault: %d", get_temperature_by_id(TC_1_ID), get_thermocouple_fault_by_id(TC_1_ID));
        ESP_LOGI(TAG, "TC2: %.2f C, fault: %d", get_temperature_by_id(TC_2_ID), get_thermocouple_fault_by_id(TC_2_ID));

        if(get_thermocouple_fault_by_id(TC_HEATER) || get_thermocouple_fault_by_id(TC_INSIDE_OVEN)) {
            ESP_LOGW(TAG, "Fault detected in one of the critical thermocouples. Disabling heating.");
            power_switch_t* power_switch_heater = get_power_switch_by_id(POWER_SWITCH_HEATER);
            power_switch_enable(power_switch_heater, 0);
            continue; // Skip the rest of the control logic if there's a fault
        }

        if (stacjolab_controller.temp_control_config.heating_enabled) {
            float current_heater_temp = get_temperature_by_id(TC_HEATER);
            float current_oven_temp = get_temperature_by_id(TC_INSIDE_OVEN);

            power_switch_t* power_switch_heater = get_power_switch_by_id(POWER_SWITCH_HEATER);

            if (current_heater_temp >= MAX_HEATER_TEMP || current_oven_temp >= stacjolab_controller.temp_control_config.high_temp_threshold){
                power_switch_enable(power_switch_heater, 0);
            }
            else if (current_heater_temp <= stacjolab_controller.temp_control_config.low_temp_threshold) {
                power_switch_enable(power_switch_heater, 1);
            }
            
        }
        else {
            power_switch_t* power_switch_heater = get_power_switch_by_id(POWER_SWITCH_HEATER);
            power_switch_enable(power_switch_heater, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }

}