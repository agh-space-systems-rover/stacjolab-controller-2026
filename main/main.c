#include <stdio.h>
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/idf_additions.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "supervisor.h"
#include "esp_now_driver.h"
#include "tlv.h"
#include "pwm_driver.h"
#include "stacjolab.h"

static const char *TAG = "main";

QueueHandle_t q_esp_now_rx;
QueueHandle_t q_esp_now_tx;
QueueHandle_t q_esp_now_rx_raw;

esp_err_t init();
esp_err_t create_tasks();
esp_err_t create_queues();

stacjolab_controller_t stacjolab_controller;

void app_main(void) {
    ESP_LOGI(TAG, "Initializing stacjolab controller");


    ESP_ERROR_CHECK(init());
    ESP_ERROR_CHECK(create_queues());
    ESP_ERROR_CHECK(create_tasks());
    ESP_ERROR_CHECK(stacjolab_controller_init(&stacjolab_controller));
    
    // Test

    h_bridge_set_speed(&stacjolab_controller.h_bridge_ch_0, 25);

    cc_driver_duty(&stacjolab_controller.cc_driver, 25);

    power_switch_set_duty(&stacjolab_controller.power_switch_ch_0, 25);
    power_switch_enable(&stacjolab_controller.power_switch_ch_0, 1);

    bool led_state = false;
    float temperature;
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        led_state = !led_state;
        gpio_set_level(LED1_PIN, led_state);
        gpio_set_level(LED2_PIN, led_state);

        thermocouple_read(&stacjolab_controller.thermocouple_manager.thermocouples[0], &temperature);
        ESP_LOGI(TAG, "Temperature 0: %.2f C", temperature);
        thermocouple_read(&stacjolab_controller.thermocouple_manager.thermocouples[1], &temperature);
        ESP_LOGI(TAG, "Temperature 1: %.2f C", temperature);
        thermocouple_read(&stacjolab_controller.thermocouple_manager.thermocouples[2], &temperature);
        ESP_LOGI(TAG, "Temperature 2: %.2f C", temperature);
    }
}

esp_err_t nvs_init() {
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    return ESP_OK;
}

esp_err_t wifi_init(uint8_t channel) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    //TODO: Add specific WiFi channel setup
    return ESP_OK;
}


esp_err_t init() {

    // Initialize NVS Flash
    nvs_init();

    // Initialize WiFi
    wifi_init(1); //TODO: set proper channel

    //print MAC address
    uint8_t mac[6];
    esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
    ESP_LOGI(TAG, "Stacjolab driver initialized. Device MAC address: %02x:%02x:%02x:%02x:%02x:%02x",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return ESP_OK;
}

esp_err_t create_tasks() {
    //TODO: set proper stack size and priority
    xTaskCreate(supervisor_task, "supervisor_task", 4096, NULL, 5, NULL);
    xTaskCreate(esp_now_task, "esp_now_task", 4096, NULL, 5, NULL);

    return ESP_OK;
}

esp_err_t create_queues() {

    q_esp_now_rx = xQueueCreate(10, sizeof(msg_t));
    q_esp_now_tx = xQueueCreate(10, sizeof(msg_t));
    q_esp_now_rx_raw = xQueueCreate(10, TLV_MAX_SIZE * sizeof(uint8_t));

    return ESP_OK;
}
