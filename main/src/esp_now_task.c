#include "esp_err.h"
#include "esp_log.h"
#include "esp_now.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"

#include "esp_now_task.h"


static const char *TAG = "esp_now_task";
void esp_now_recv_cb(const esp_now_recv_info_t * esp_now_info, const uint8_t *data, int data_len);

void esp_now_task(void *arg) {
    ESP_ERROR_CHECK(esp_now_init());

    esp_now_register_recv_cb(esp_now_recv_cb);
    
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void esp_now_recv_cb(const esp_now_recv_info_t * esp_now_info, const uint8_t *data, int data_len) {
    ESP_LOGI(TAG, "Received ESPNOW data from: %02x:%02x:%02x:%02x:%02x:%02x, len: %d",
             esp_now_info->src_addr[0], esp_now_info->src_addr[1], esp_now_info->src_addr[2],
             esp_now_info->src_addr[3], esp_now_info->src_addr[4], esp_now_info->src_addr[5],
             data_len);
    // Process received data
    for(int i = 0; i < data_len; i++) {
        printf("%02x ", data[i]);
    }
    printf("\n");
}