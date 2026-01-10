#include "supervisor.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


static const char *TAG = "supervisor";

void supervisor_task(void *arg) {
    while (1) {
        ESP_LOGI(TAG, "Supervisor task running");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}