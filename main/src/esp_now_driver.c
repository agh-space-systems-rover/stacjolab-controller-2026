#include "esp_err.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi_types_generic.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "portmacro.h"
#include "tlv.h"
#include <sys/types.h>

#include "esp_now_driver.h"


static const char *TAG = "ESP_NOW";
static const uint8_t peer_addr[] = {0x48, 0x27, 0xE2, 0x14, 0xab, 0xC4};

void esp_now_register_callbacks();

void esp_now_task(void *arg) {

    esp_now_init();
    esp_now_register_callbacks();
    esp_now_peer_info_t peer = {
        .channel = 1,
        .encrypt = false,
        .ifidx = ESP_IF_WIFI_STA,
    };

    memcpy(peer.peer_addr, peer_addr, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(&peer));
    
    uint8_t data[TLV_MAX_SIZE];
    msg_t msg;

    while (1) {
        if(xQueueReceive(q_esp_now_rx_raw, data, 0) == pdTRUE) {
            ESP_LOGI(TAG, "Processing received ESPNOW data, first byte: %02x", data[0]);

            if(tlv_deserialize(data, TLV_MAX_SIZE, &msg) != 0) {
                ESP_LOGI(TAG, "Serialized TLV type: %02x, length: %d", msg.type, msg.length);
                xQueueSend(q_esp_now_rx, &msg, portMAX_DELAY);

            } else {
                ESP_LOGE(TAG, "Failed to deserialize TLV");
            }
        }

        if(xQueueReceive(q_esp_now_tx, &msg, 0) == pdTRUE) {
            ESP_LOGI(TAG, "Sending TLV type: %02x, length: %d", msg.type, msg.length);
            size_t serialized_len = tlv_serialize(data, TLV_MAX_SIZE, &msg);
            if(serialized_len > 0) {
                esp_now_send(peer_addr, data, serialized_len); // NULL for broadcast
            } else {
                ESP_LOGE(TAG, "Failed to serialize TLV");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void esp_now_recv_cb(const esp_now_recv_info_t * esp_now_info, const uint8_t *data, int data_len) {
    ESP_LOGI(TAG, "Received ESPNOW data from: %02x:%02x:%02x:%02x:%02x:%02x, len: %d",
             esp_now_info->src_addr[0], esp_now_info->src_addr[1], esp_now_info->src_addr[2],
             esp_now_info->src_addr[3], esp_now_info->src_addr[4], esp_now_info->src_addr[5],
             data_len);

    xQueueSendFromISR(q_esp_now_rx_raw, data, NULL);
}

void esp_now_send_cb(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
    ESP_LOGI(TAG, "ESPNOW data sent to: %02x:%02x:%02x:%02x:%02x:%02x, status: %s",
             tx_info->des_addr[0], tx_info->des_addr[1], tx_info->des_addr[2],
             tx_info->des_addr[3], tx_info->des_addr[4], tx_info->des_addr[5],
             status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAILURE");
}

void esp_now_register_callbacks() {
    esp_now_register_recv_cb(esp_now_recv_cb);
    esp_now_register_send_cb(esp_now_send_cb);
}

