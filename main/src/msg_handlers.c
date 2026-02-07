#include "msg_handlers.h"
#include "stacjolab.h"

#include "freertos/idf_additions.h"


void on_ping(const msg_t *msg, void *user_ctx) {
    msg_t response;
    response.type = 0x01; // Pong message type
    response.length = 0;

    // Prepare pong response (no payload)

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