#include "supervisor.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


static const char *TAG = "supervisor";

static void supervisor_status_report();

void supervisor_task(void *arg) {
    while (1) {
        supervisor_status_report();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void supervisor_status_report() {
    ESP_LOGI(TAG, "Supervisor status report");
}