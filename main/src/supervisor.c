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
    dispatcher_register_handler(HEATER_MSG_TYPE, on_heater_enable, NULL);
    dispatcher_register_handler(HEATER_CONFIG_MSG_TYPE, on_heater_config, NULL);
    dispatcher_register_handler(GET_TC_TEMP_MSG_TYPE, on_get_tc_temp, NULL);
    dispatcher_register_handler(LED_STRIP_SET_SOLID_MSG_TYPE, on_led_strip_set_solid, NULL);
    dispatcher_register_handler(LED_STRIP_SET_SINGLE_MSG_TYPE, on_led_strip_set_single, NULL);
    dispatcher_register_handler(H_BRIDGE_MSG_TYPE, on_h_bridge_set_speed, NULL);
    dispatcher_register_handler(CC_DRIVER_MSG_TYPE, on_cc_driver_set_duty, NULL);
    dispatcher_register_handler(WEIGHT_REQ_MSG_TYPE, on_weight_req, NULL);
    dispatcher_register_handler(WEIGHT_TARE_MSG_TYPE, on_weight_tare, NULL);

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