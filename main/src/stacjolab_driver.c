#include "stacjolab_driver.h"
#include "supervisor.h"

esp_err_t init() {
    // Initialization code here
    return ESP_OK;
}

esp_err_t create_tasks() {
    xTaskCreate(supervisor_task, "supervisor_task", 4096, NULL, 5, NULL);

    return ESP_OK;
}