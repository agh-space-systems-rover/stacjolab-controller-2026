#include <string.h>

#include "stacjolab.h"
#include "thermocouple.h"
#include "pin_def.h"

#include "esp_log.h"

static const char* TAG = "thermocouple";

uint8_t tc_cs_pins[TC_COUNT] = {TC_nCS1_PIN, TC_nCS2_PIN, TC_nCS3_PIN};

#define TC_TEMP_MASK ((1 << 14) - 1)
#define TC_TEMP_OFFSET 18

#define TC_INTERNAL_TEMP_MASK ((1 << 16) - 1)
#define TC_INTERNAL_TEMP_OFFSET 4

#define TC_OC_BIT 0
#define TC_SCG_BIT 1
#define TC_SCV_BIT 2
#define TC_FAULT_BIT 16

#define FAULT_READING -67.0f

esp_err_t thermocouple_manager_init(thermocouple_manager_t* manager) {

    esp_err_t ret;

    spi_bus_config_t bus_config = {
        .mosi_io_num = -1,
        .miso_io_num = TC_MISO_PIN,
        .sclk_io_num = TC_SCK_PIN,
        .quadhd_io_num = -1,
        .quadwp_io_num = -1,
        .max_transfer_sz = 4096,
    };

    ret = spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    for(int i = 0; i < TC_COUNT; i++) {
        ESP_ERROR_CHECK(thermocouple_init(&manager->thermocouples[i], i));
    }

    ESP_LOGI(TAG, "Thermocouple manager initialized with %d thermocouples", TC_COUNT);

    return ESP_OK;
}

esp_err_t thermocouple_init(thermocouple_t* thermocouple, uint8_t id) {
    
    esp_err_t ret;

    thermocouple->id = id;

    memset(thermocouple->filter_buffer, 0, sizeof(thermocouple->filter_buffer));
    thermocouple->filter_newest_reading = 0;
    thermocouple->filtered_temperature = 0.0f;
    thermocouple->fault_detected = false;

    spi_device_interface_config_t dev_config = {
        .clock_speed_hz = 1 * 1000 * 1000, // 1 MHz
        .mode = 0,
        .spics_io_num = tc_cs_pins[thermocouple->id],
        .queue_size = 1,
    };

    ret = spi_bus_add_device(SPI2_HOST, &dev_config, &thermocouple->spi_handle);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Thermocouple %d initialized", thermocouple->id);

    return ESP_OK;
}

thermocouple_t* get_thermocouple_by_id(uint8_t id){
    switch(id){
        case TC_0_ID:
            return &stacjolab_controller.thermocouple_manager.thermocouples[0];
        case TC_1_ID:
            return &stacjolab_controller.thermocouple_manager.thermocouples[1];
        case TC_2_ID:
            return &stacjolab_controller.thermocouple_manager.thermocouples[2];
        default:
            ESP_LOGW(TAG, "Invalid thermocouple ID: %d", id);
            return NULL; // Return true for invalid ID to indicate fault
    }
}

static void reverse_bytes(uint8_t* data, size_t length) {
    for (size_t i = 0; i < length / 2; i++) {
        uint8_t temp = data[i];
        data[i] = data[length - 1 - i];
        data[length - 1 - i] = temp;
    }
}

static float convert_thermocouple_temperature(thermocouple_t* thermocouple) {
    int32_t thermocouple_temp = *(int32_t*)thermocouple->register_data >> TC_TEMP_OFFSET;
    return (float)thermocouple_temp * 0.25f;
}

static float convert_internal_temperature(thermocouple_t* thermocouple) {
    int16_t internal_temp = *(int16_t*)thermocouple->register_data & TC_INTERNAL_TEMP_MASK >> TC_INTERNAL_TEMP_OFFSET;
    return (float)internal_temp * 0.0625f;
}

static float moving_average(float* buffer, size_t size) {
    float sum = 0.0f;
    for(size_t i = 0; i < size; i++) {
        sum += buffer[i];
    }
    return sum / (float)size;
}

esp_err_t thermocouple_read(thermocouple_t* thermocouple) {

    esp_err_t ret;

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 32;
    t.rx_buffer = thermocouple->register_data;
    // ret = spi_device_transmit(thermocouple->spi_handle, &t);
    ret = spi_device_polling_transmit(thermocouple->spi_handle, &t);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read from SPI device: %s", esp_err_to_name(ret));
        return ret;
    }

    reverse_bytes(thermocouple->register_data, 4);

    // ESP_LOGI(TAG, "Read data from SPI device: D3=%02X D2=%02X D1=%02X D0=%02X",
    //          thermocouple->register_data[3], thermocouple->register_data[2],
    //          thermocouple->register_data[1], thermocouple->register_data[0]);   

    if (thermocouple->register_data[2] & 0x1) {
        thermocouple->flags.OC = (*thermocouple->register_data >> TC_OC_BIT) & 0x1;
        thermocouple->flags.SCG = (*thermocouple->register_data >> TC_SCG_BIT) & 0x1;
        thermocouple->flags.SCV = (*thermocouple->register_data >> TC_SCV_BIT) & 0x1;

        // ESP_LOGW(TAG, "Thermocouple fault detected: OC=%d, SCG=%d, SCV=%d", thermocouple->flags.OC, thermocouple->flags.SCG, thermocouple->flags.SCV);

        thermocouple->fault_detected = true;

    }
    else {

        // ESP_LOGI(TAG, "Thermocouple temperature: %.2f C", convert_thermocouple_temperature(thermocouple));
        // ESP_LOGI(TAG, "Internal temperature: %.2f", convert_internal_temperature(thermocouple));

        thermocouple->filter_newest_reading = (thermocouple->filter_newest_reading + 1) % TC_FILTER_SIZE;
        thermocouple->filter_buffer[thermocouple->filter_newest_reading] = convert_thermocouple_temperature(thermocouple);
        thermocouple->filtered_temperature = moving_average(thermocouple->filter_buffer, TC_FILTER_SIZE);

        thermocouple->fault_detected = false;

    }

    return ESP_OK;
}

esp_err_t thermocouple_read_all(thermocouple_manager_t* manager) {
    for(int i = 0; i < TC_COUNT; i++) {
        esp_err_t ret = thermocouple_read(&manager->thermocouples[i]);
        if(ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read thermocouple %d: %s", i, esp_err_to_name(ret));
            return ret;
        }
    }
    return ESP_OK;
}

bool get_thermocouple_fault_by_id(uint8_t id) {
    thermocouple_t* tc = get_thermocouple_by_id(id);
    if (tc == NULL) {
        ESP_LOGW(TAG, "Invalid thermocouple ID: %d", id);
        return true; // Return true for invalid ID to indicate fault
    }
    return tc->fault_detected;
}

float get_filtered_temperature(thermocouple_t* thermocouple) {
    if (thermocouple->fault_detected) {
        return FAULT_READING;
    }
    return thermocouple->filtered_temperature;
}

float get_raw_temperature(thermocouple_t* thermocouple) {
    return thermocouple->filter_buffer[thermocouple->filter_newest_reading];
}

float get_temperature_by_id(uint8_t id) {
    thermocouple_t* tc = get_thermocouple_by_id(id);
    if (tc == NULL) {
        ESP_LOGW(TAG, "Invalid thermocouple ID: %d", id);
    }
    return get_filtered_temperature(tc);
}