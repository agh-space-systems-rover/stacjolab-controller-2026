#include "msg_handlers.h"
#include "freertos/idf_additions.h"


void on_ping(const msg_t *msg, void *user_ctx) {
    msg_t response;
    response.type = 0x01; // Pong message type
    response.length = 0;

    // Prepare pong response (no payload)

    xQueueSend(q_esp_now_tx, &response, pdMS_TO_TICKS(100));
}