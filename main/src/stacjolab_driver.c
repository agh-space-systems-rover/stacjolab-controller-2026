#include "stacjolab_driver.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "supervisor.h"
#include "esp_now_task.h"

esp_err_t init() {

    // Initialize NVS Flash
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize WiFi
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    //TODO: Add specific WiFi channel setup


    return ESP_OK;
}

esp_err_t create_tasks() {
    //TODO: set proper stack size and priority
    xTaskCreate(supervisor_task, "supervisor_task", 4096, NULL, 5, NULL);
    xTaskCreate(esp_now_task, "esp_now_task", 4096, NULL, 5, NULL);

    return ESP_OK;
}
