#include "esp_log.h"
#include "msg_handlers.h"


char* TAG_S = "STATION";


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