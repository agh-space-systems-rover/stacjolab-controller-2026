#include "supervisor.h"
#include "dispatcher.h"
#include "esp_log.h"
#include "esp_now_driver.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/idf_additions.h"
#include "freertos/task.h"
#include "msg_handlers.h"
#include "tlv.h"
#include "msg_handlers.h"


static const char *TAG = "supervisor";

static void supervisor_status_report();

void supervisor_task(void *arg) {

    msg_t msg;

    dispatcher_init();
    dispatcher_register_handler(0x00, on_ping, NULL);

    while (1) {
        supervisor_status_report();


        if(xQueueReceive(q_esp_now_rx, &msg, 0) == pdTRUE) {
            ESP_LOGI(TAG, "Supervisor received TLV type: %02x, length: %d", msg.type, msg.length);

            dispatcher_dispatch(&msg);
        }


        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void supervisor_status_report() {
    ESP_LOGI(TAG, "Supervisor status report");
}