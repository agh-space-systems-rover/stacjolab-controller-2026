#include "msg_handlers.h"
#include "stacjolab.h"
#include "tensometer.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include <string.h>


static const char *TAG = "MSG";

void on_ping(const msg_t *msg, void *user_ctx) {
    msg_t response;
    response.type = 0x01; // Pong message type
    response.length = 0;

    // Prepare pong response (no payload)

    xQueueSend(q_esp_now_tx, &response, pdMS_TO_TICKS(100));
}

void on_heater_enable(const msg_t *msg, void *user_ctx) {
    if(msg->length != sizeof(heater_msg_t)) {
        // Invalid message length
        return;
    }

    heater_msg_t *heater_msg = (heater_msg_t *)msg->payload;

    stacjolab_controller.temp_control_config.heating_enabled = heater_msg->enabled;
}

void on_heater_config(const msg_t *msg, void *user_ctx) {
    if(msg->length != sizeof(heater_config_msg_t)) {
        // Invalid message length
        return;
    }

    heater_config_msg_t *config_msg = (heater_config_msg_t *)msg->payload;

    stacjolab_controller.temp_control_config.high_temp_threshold = config_msg->high_temp_threshold;
    stacjolab_controller.temp_control_config.low_temp_threshold = config_msg->low_temp_threshold;
    stacjolab_controller.temp_control_config.heater_duty_cycle = config_msg->heater_duty_cycle;
}

void on_get_tc_temp(const msg_t *msg, void *user_ctx) {
    if(msg->length != sizeof(get_tc_temp_msg_t)) {
        // Invalid message length
        return;
    }

    get_tc_temp_msg_t *get_temp_msg = (get_tc_temp_msg_t *)msg->payload;

    uint8_t tc_id = get_temp_msg->tc_id;
    float temp = get_temperature_by_id(tc_id);
    bool fault = get_thermocouple_fault_by_id(tc_id);

    resp_tc_temp_msg_t resp_msg;
    resp_msg.tc_id = tc_id;
    resp_msg.temp = (int16_t)(temp * 100); // Convert to fixed-point representation
    resp_msg.fault = fault ? 1 : 0;

    msg_t response;
    response.type = RESP_TC_TEMP_MSG_TYPE;
    response.length = sizeof(resp_tc_temp_msg_t);
    memcpy(response.payload, &resp_msg, sizeof(resp_tc_temp_msg_t));

    xQueueSend(q_esp_now_tx, &response, pdMS_TO_TICKS(100));
}

void on_h_bridge_set_speed(const msg_t *msg, void *user_ctx) {
    if(msg->length != sizeof(h_bridge_msg_t)) {
        // Invalid message length
        return;
    }

    h_bridge_msg_t *h_bridge_msg = (h_bridge_msg_t *)msg->payload;

    h_bridge_t* h_bridge = get_h_bridge_by_id(h_bridge_msg->h_bridge_id);
    if(h_bridge != NULL) {
        h_bridge_set_speed(h_bridge, h_bridge_msg->speed);
    }
}

void on_cc_driver_set_duty(const msg_t *msg, void *user_ctx) {
    if(msg->length != sizeof(cc_driver_msg_t)) {
        // Invalid message length
        return;
    }

    cc_driver_msg_t *cc_msg = (cc_driver_msg_t *)msg->payload;

    cc_driver_duty(&stacjolab_controller.cc_driver, cc_msg->duty);
}

void on_weight_req(const msg_t *msg, void *user_ctx) {
    (void)user_ctx;
    if(msg->length != 0) {
        return;
    }

    uint64_t sum = 0;
    for(uint8_t i = 0; i < TENSOMETER_MOVING_AVERAGE_SIZE; i++) {
        tensometer_read_all();
        sum += tensometer_get_voltage_sum();
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    int32_t voltage_uv = sum / TENSOMETER_MOVING_AVERAGE_SIZE;
    msg_t response;
    response.type = WEIGHT_RESP_MSG_TYPE; // 0xD1
    response.length = sizeof(weight_resp_msg_t); // 4 bytes
    
    weight_resp_msg_t *resp_payload = (weight_resp_msg_t *)response.payload;
    resp_payload->weight = voltage_uv;
    for(int i = 0; i < response.length; i++) {
        ESP_LOGI(TAG, "Response payload byte %d: %02x", i, response.payload[i]);
    }
    xQueueSend(q_esp_now_tx, &response, pdMS_TO_TICKS(100));
}

void on_weight_tare(const msg_t *msg, void *user_ctx) {
    (void)user_ctx;
    if(msg->length != 0) {
        return;
    }
    tensometer_tare();
}