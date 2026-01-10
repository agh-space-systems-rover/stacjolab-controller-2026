#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#include "stacjolab_driver.h"


static const char *TAG = "main";

void app_main(void) {
    ESP_LOGI(TAG, "Initializing stacjolab controller");


    ESP_ERROR_CHECK(init());
    ESP_ERROR_CHECK(create_tasks());

    while(1) {
        ESP_LOGI(TAG, "Running main loop");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
