#include "esp_err.h"
#include "esp_log.h"

#include "uart_if.h"
#include "tlv.h"

static const char *TAG = "uart_if";

#define BUFFER_SIZE 1024

extern QueueHandle_t q_esp_now_tx;

void uart_task(void *arg)
{

    usb_serial_jtag_driver_config_t config = {
        .rx_buffer_size = BUFFER_SIZE,
        .tx_buffer_size = BUFFER_SIZE,
    };

    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&config));
    ESP_LOGI(TAG, "UART initialized with RX buffer size: %d and TX buffer size: %d", config.rx_buffer_size, config.tx_buffer_size);

    uint8_t *data = (uint8_t *) malloc(BUFFER_SIZE);
    if (data == NULL) {
        ESP_LOGE("usb_serial_jtag echo", "no memory for data");
        return;
    }

    msg_t msg;

    while (1) {

        int len = usb_serial_jtag_read_bytes(data, (BUFFER_SIZE - 1), portMAX_DELAY);
        // Write data back to the USB SERIAL JTAG

        if (len) {
            data[len] = '\0';
            ESP_LOG_BUFFER_HEXDUMP("Recv str: ", data, len, ESP_LOG_INFO);

            msg.type = data[0];
            msg.length = data[1];
            if (msg.length > 0) {
                memcpy(msg.payload, &data[2], msg.length);
            }

            ESP_LOGI(TAG, "Received message - Type: %d, Length: %d", msg.type, msg.length);

            xQueueSend(q_esp_now_tx, &msg, portMAX_DELAY);
        }

    }
}