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

#include "uart_if.h"
// #include "adc_external_driver.h"
#include "pin_def.h"
#include "tensometer.h" // Include tensometer.h

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
    ESP_ERROR_CHECK(stacjolab_controller_init(&stacjolab_controller));
    ESP_ERROR_CHECK(create_queues());

    // Create Tensometer Task via tensometer module
    tensometer_init(TENSO_SDA_PIN, TENSO_SCL_PIN);

    ESP_ERROR_CHECK(create_tasks());
    
    
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
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
    xTaskCreate(uart_task, "uart_task", 4096, NULL, 5, NULL);
    xTaskCreate(temp_control_task, "temp_control_task", 4096, NULL, 5, NULL);
    xTaskCreate(temp_read_task, "temp_read_task", 4096, NULL, 5, NULL);
    // xTaskCreate(tensometer_task, "tensometer_task", 4096, NULL, 5, NULL);

    return ESP_OK;
}

esp_err_t create_queues() {

    q_esp_now_rx = xQueueCreate(10, sizeof(msg_t));
    q_esp_now_tx = xQueueCreate(10, sizeof(msg_t));
    q_esp_now_rx_raw = xQueueCreate(10, TLV_MAX_SIZE * sizeof(uint8_t));

    return ESP_OK;
}
