#include "stacjolab.h"

#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_err.h"

static const char* TAG = "stacjolab_controller";

esp_err_t stacjolab_controller_init(stacjolab_controller_t* controller) {
    if(controller == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Initializing stacjolab controller");

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
    controller->temp_control_config.high_temp_threshold = CONFIG_STACJOLAB_HIGH_TEMP_THRESHOLD;
    controller->temp_control_config.low_temp_threshold = CONFIG_STACJOLAB_LOW_TEMP_THRESHOLD;
    controller->temp_control_config.heater_duty_cycle = CONFIG_STACJOLAB_HEATER_DUTY_CYCLE;
#ifdef CONFIG_STACJOLAB_HEATING_ENABLED
    controller->temp_control_config.heating_enabled = true;
#else
    controller->temp_control_config.heating_enabled = false;
#endif

    ESP_ERROR_CHECK(led_strip_init(&controller->led_strip));

#ifdef CONFIG_STACJOLAB_LED_ENABLED_ON_BOOT
    led_strip_set_solid_color(&controller->led_strip, CONFIG_STACJOLAB_RED_ON_BOOT, CONFIG_STACJOLAB_GREEN_ON_BOOT, CONFIG_STACJOLAB_BLUE_ON_BOOT);
#else
    led_strip_set_solid_color(&controller->led_strip, 0, 0, 0);
#endif
    
    servo_config_t servo_config = {
        .max_angle = SERVO_0_MAX_ANGLE,
        .min_width_us = SERVO_0_MIN_PULSE_WIDTH_US,
        .max_width_us = SERVO_0_MAX_PULSE_WIDTH_US,
        .channel_number = SERVO_COUNT,
        .channels = {
            .ch = {SERVO_0_CHANNEL},
            .servo_pin = {SERVO_0_PIN}
        },
        .freq = LEDC_TIMER_1_FREQ,
        .timer_number = LEDC_TIMER_1
    };
    ESP_ERROR_CHECK(iot_servo_init(LEDC_LOW_SPEED_MODE, &servo_config));
    
    ESP_LOGI(TAG, "Controller initialized");

    return ESP_OK;
}

void temp_read_task(void *arg) {
    while (1) {
        thermocouple_read_all(&stacjolab_controller.thermocouple_manager);

        ESP_LOGI(TAG, "TC0: %.2f C, fault: %d", get_temperature_by_id(TC_0_ID), get_thermocouple_fault_by_id(TC_0_ID));
        ESP_LOGI(TAG, "TC1: %.2f C, fault: %d", get_temperature_by_id(TC_1_ID), get_thermocouple_fault_by_id(TC_1_ID));
        ESP_LOGI(TAG, "TC2: %.2f C, fault: %d", get_temperature_by_id(TC_2_ID), get_thermocouple_fault_by_id(TC_2_ID));

        vTaskDelay(pdMS_TO_TICKS(TEMP_READ_TASK_INTERVAL_MS));
    }
}

void temp_control_task(void *arg) {

    
    while (1) {

        gpio_set_level(LED2_PIN, stacjolab_controller.power_switch_ch_0.enabled);
        gpio_set_level(LED1_PIN, stacjolab_controller.temp_control_config.heating_enabled);

        // thermocouple_read_all(&stacjolab_controller.thermocouple_manager);

        // ESP_LOGI(TAG, "TC0: %.2f C, fault: %d", get_temperature_by_id(TC_0_ID), get_thermocouple_fault_by_id(TC_0_ID));
        // ESP_LOGI(TAG, "TC1: %.2f C, fault: %d", get_temperature_by_id(TC_1_ID), get_thermocouple_fault_by_id(TC_1_ID));
        // ESP_LOGI(TAG, "TC2: %.2f C, fault: %d", get_temperature_by_id(TC_2_ID), get_thermocouple_fault_by_id(TC_2_ID));

        ESP_LOGI(TAG, "Temp control config - High: %.2f C, Low: %.2f C, Duty: %.2f %%, Enabled: %d",
                stacjolab_controller.temp_control_config.high_temp_threshold,
                stacjolab_controller.temp_control_config.low_temp_threshold,
                stacjolab_controller.temp_control_config.heater_duty_cycle,
                stacjolab_controller.temp_control_config.heating_enabled);

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
                ESP_LOGI(TAG, "Temperature threshold exceeded. Disabling heater.");
                power_switch_enable(power_switch_heater, 0);
                ESP_LOGI(TAG, "Current heater temp: %.2f C, Current oven temp: %.2f C", current_heater_temp, current_oven_temp);
            }
            else if (current_oven_temp <= stacjolab_controller.temp_control_config.low_temp_threshold) {
                ESP_LOGI(TAG, "Temperature below threshold. Enabling heater.");
                power_switch_set_duty(power_switch_heater, stacjolab_controller.temp_control_config.heater_duty_cycle);
                power_switch_enable(power_switch_heater, 1);
                ESP_LOGI(TAG, "Heater duty cycle set to %.2f %%", stacjolab_controller.temp_control_config.heater_duty_cycle);
            }
            
        }
        else {
            power_switch_t* power_switch_heater = get_power_switch_by_id(POWER_SWITCH_HEATER);
            power_switch_enable(power_switch_heater, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(TEMP_CONTROL_TASK_INTERVAL_MS));
    }

}