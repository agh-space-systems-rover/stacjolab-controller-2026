#include "supervisor.h"
#include "esp_log.h"

#include "esp_now_driver.h"

#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/idf_additions.h"
#include "freertos/task.h"
#include "portmacro.h"
#include "tlv.h"


static const char *TAG = "supervisor";

static void supervisor_status_report();

void supervisor_task(void *arg) {

    msg_t msg;

    while (1) {
        supervisor_status_report();


        if(xQueueReceive(q_esp_now_rx, &msg, 0) == pdTRUE) {
            ESP_LOGI(TAG, "Supervisor received TLV type: %02x, length: %d", msg.type, msg.length);

            //TODO: write dispatcher, for now just echo back


            xQueueSend(q_esp_now_tx, &msg, portMAX_DELAY);
        }


        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void supervisor_status_report() {
    ESP_LOGI(TAG, "Supervisor status report");
}