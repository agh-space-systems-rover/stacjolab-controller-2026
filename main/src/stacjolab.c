#include "stacjolab.h"

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
    gpio_set_level(LED1_PIN, 1);

    // UNSED COMPONENTS FOR NOW
    // // Initialize H-Bridge channels
    // ESP_ERROR_CHECK(h_bridge_init(&controller->h_bridge_ch_1, H_BRIDGE_1_ID));
    // ESP_ERROR_CHECK(h_bridge_init(&controller->h_bridge_ch_0, H_BRIDGE_0_ID));

    // // Initialize CC driver
    // ESP_ERROR_CHECK(cc_driver_init(&controller->cc_driver));

    // Initialize power switches
    // Heater
    ESP_ERROR_CHECK(power_switch_init(&controller->power_switch_ch_0, POWER_SWITCH_0_ID, LEDC_TIMER_1, LEDC_CHANNEL_0, PS_PWM_1_PIN, LEDC_TIMER_1_FREQ, LEDC_TIMER_1_RESOLUTION));
    // Pump
    ESP_ERROR_CHECK(power_switch_init(&controller->power_switch_ch_1, POWER_SWITCH_1_ID, LEDC_TIMER_2, LEDC_CHANNEL_1, PS_PWM_2_PIN, LEDC_TIMER_2_FREQ, LEDC_TIMER_2_RESOLUTION));
    // UNUSED
    ESP_ERROR_CHECK(power_switch_init(&controller->power_switch_ch_2, POWER_SWITCH_2_ID, LEDC_TIMER_1, LEDC_CHANNEL_2, PS_PWM_3_PIN, LEDC_TIMER_1_FREQ, LEDC_TIMER_1_RESOLUTION));
    
    ESP_ERROR_CHECK(thermocouple_manager_init(&controller->thermocouple_manager));

    // Initialize temperature control config
    controller->temp_control_config.high_temp_threshold = STACJOLAB_HIGH_TEMP_THRESHOLD;
    controller->temp_control_config.low_temp_threshold = STACJOLAB_LOW_TEMP_THRESHOLD;
    controller->temp_control_config.heater_duty_cycle = STACJOLAB_HEATER_DUTY_CYCLE;
    controller->temp_control_config.lid_heater_duty_cycle = STACJOLAB_LID_HEATER_DUTY_CYCLE;
    controller->temp_control_config.heating_enabled = false;

    ESP_ERROR_CHECK(led_strip_init(&controller->led_strip));
    led_strip_set_solid_color(&controller->led_strip, STACJOLAB_RED_ON_BOOT, STACJOLAB_GREEN_ON_BOOT, STACJOLAB_BLUE_ON_BOOT);
    ESP_LOGI(TAG, "LED strip initialized with color R:%d G:%d B:%d", STACJOLAB_RED_ON_BOOT, STACJOLAB_GREEN_ON_BOOT, STACJOLAB_BLUE_ON_BOOT);    

    ESP_ERROR_CHECK(servo_init(&controller->servo, AUX_1_PIN, LEDC_TIMER_0, LEDC_CHANNEL_3, LEDC_TIMER_0_FREQ, LEDC_TIMER_0_RESOLUTION));
    ESP_LOGI(TAG, "Servo initialized on channel %d", LEDC_CHANNEL_3);

    tensometer_init(TENSO_SDA_PIN, TENSO_SCL_PIN);
    ESP_LOGI(TAG, "Tensometer initialized");

    gpio_set_level(LED1_PIN, 0);
    ESP_LOGI(TAG, "Controller initialized");

    return ESP_OK;
}

void temp_read_task(void *arg) {
    while (1) {
        thermocouple_read_all(&stacjolab_controller.thermocouple_manager);

        static TickType_t last_log = 0;
        TickType_t now = xTaskGetTickCount();
        if ((now - last_log) > pdMS_TO_TICKS(THERMOCOUPLE_LOG_TIME)) {
            ESP_LOGI(TAG, "TC0: %.2f C, fault: %d", get_temperature_by_id(TC_0_ID), get_thermocouple_fault_by_id(TC_0_ID));
            ESP_LOGI(TAG, "TC1: %.2f C, fault: %d", get_temperature_by_id(TC_1_ID), get_thermocouple_fault_by_id(TC_1_ID));
            ESP_LOGI(TAG, "TC2: %.2f C, fault: %d", get_temperature_by_id(TC_2_ID), get_thermocouple_fault_by_id(TC_2_ID));
            last_log = now;
        }

        vTaskDelay(pdMS_TO_TICKS(TEMP_READ_TASK_INTERVAL_MS));
    }
}

void temp_control_task(void *arg) {

    
    while (1) {

        gpio_set_level(LED2_PIN, stacjolab_controller.power_switch_ch_0.enabled);
        gpio_set_level(LED1_PIN, stacjolab_controller.temp_control_config.heating_enabled);

        ESP_LOGI(TAG, "Temp control config - High: %.2f C, Low: %.2f C, Duty: %.2f %%, Enabled: %d",
                stacjolab_controller.temp_control_config.high_temp_threshold,
                stacjolab_controller.temp_control_config.low_temp_threshold,
                stacjolab_controller.temp_control_config.heater_duty_cycle,
                stacjolab_controller.temp_control_config.heating_enabled);


        if (stacjolab_controller.temp_control_config.heating_enabled) {
            
            if(get_thermocouple_fault_by_id(TC_HEATER) || get_thermocouple_fault_by_id(TC_INSIDE_OVEN)) {
                ESP_LOGW(TAG, "Fault detected in one of the critical thermocouples. Disabling heating.");
                power_switch_t* power_switch_heater = get_power_switch_by_id(POWER_SWITCH_HEATER);
                power_switch_enable(power_switch_heater, 0);
                continue; // Skip the rest of the control logic if there's a fault
            }

            float current_heater_temp = get_temperature_by_id(TC_HEATER);
            float current_oven_temp = get_temperature_by_id(TC_INSIDE_OVEN);

            power_switch_t* power_switch_heater = get_power_switch_by_id(POWER_SWITCH_HEATER);
            power_switch_t* power_switch_lid_heater = get_power_switch_by_id(POWER_SWITCH_LID_HEATER);

            if (current_heater_temp >= MAX_HEATER_TEMP || current_oven_temp >= stacjolab_controller.temp_control_config.high_temp_threshold){
                power_switch_enable(power_switch_heater, 0);
                power_switch_enable(power_switch_lid_heater, 0);
                ESP_LOGI(TAG, "Temperature threshold exceeded. Disabling heater. Current heater temp: %.2f C, Current oven temp: %.2f C", current_heater_temp, current_oven_temp);
            }
            else if (current_oven_temp <= stacjolab_controller.temp_control_config.low_temp_threshold) {
                power_switch_set_duty(power_switch_heater, stacjolab_controller.temp_control_config.heater_duty_cycle);
                power_switch_set_duty(power_switch_lid_heater, stacjolab_controller.temp_control_config.lid_heater_duty_cycle);
                power_switch_enable(power_switch_heater, 1);
                power_switch_enable(power_switch_lid_heater, 1);
                ESP_LOGI(TAG, "Temperature below threshold. Enabling heater. Heater duty cycle set to %.2f %%, Lid heater duty cycle set to %.2f %%", stacjolab_controller.temp_control_config.heater_duty_cycle, stacjolab_controller.temp_control_config.lid_heater_duty_cycle);
            }
            
        }
        else {
            power_switch_t* power_switch_heater = get_power_switch_by_id(POWER_SWITCH_HEATER);
            power_switch_enable(power_switch_heater, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(TEMP_CONTROL_TASK_INTERVAL_MS));
    }

}