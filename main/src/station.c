#include "esp_log.h"
#include "msg_handlers.h"


char* TAG_S = "STATION";


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

void on_resp_tc_temp(const msg_t *msg, void *user_ctx)
{
    (void)user_ctx;
    if(msg->length != 6) {
        return;
    }

    resp_tc_temp_msg_t *tc_temp_msg = (resp_tc_temp_msg_t *)msg->payload;

    ESP_LOGI(TAG_S, "Received TC temp response: tc_id=%d, temp=%d, fault=%d", tc_temp_msg->tc_id, tc_temp_msg->temp, tc_temp_msg->fault);

}


void on_resp_tc_all_temp(const msg_t *msg, void *user_ctx)
{
    (void)user_ctx;
    if(msg->length != 14) {
        return;
    }

    resp_tc_temp_msg_t *tc_temp_msg = (resp_tc_temp_msg_t *)msg->payload;

    for(int i = 0; i < 3; i++) {
        ESP_LOGI(TAG_S, "TC%d: temp=%d, fault=%d", tc_temp_msg[i].tc_id, tc_temp_msg[i].temp, tc_temp_msg[i].fault);
    }
}


void on_weight_resp(const msg_t *msg, void *user_ctx)
{
    (void)user_ctx;
    if(msg->length != 4) {
        return;
    }

    weight_resp_msg_t *tenso_msg = (weight_resp_msg_t *)msg->payload;

    ESP_LOGI(TAG_S, "Received weight response: %d uV", tenso_msg->weight);

}