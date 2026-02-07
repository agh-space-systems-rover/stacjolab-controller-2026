#include <string.h>

#include "thermocouple.h"
#include "pin_def.h"

#include "esp_log.h"

static const char* TAG = "thermocouple";

#define TC_COUNT 3

uint8_t tc_cs_pins[TC_COUNT] = {TC_nCS1_PIN, TC_nCS2_PIN, TC_nCS3_PIN};

#define TC_TEMP_MASK ((1 << 14) - 1)
#define TC_TEMP_OFFSET 18

#define TC_INTERNAL_TEMP_MASK ((1 << 16) - 1)
#define TC_INTERNAL_TEMP_OFFSET 4

#define TC_OC_BIT 0
#define TC_SCG_BIT 1
#define TC_SCV_BIT 2
#define TC_FAULT_BIT 16

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

    return ESP_OK;
}

esp_err_t thermocouple_init(thermocouple_t* thermocouple, uint8_t id) {
    
    esp_err_t ret;

    thermocouple->id = id;

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

    return ESP_OK;
}

static float convert_thermocouple_temperature(thermocouple_t* thermocouple) {
    int32_t thermocouple_temp = (int32_t)thermocouple->register_data >> TC_TEMP_OFFSET;
    return (float)thermocouple_temp * 0.25f;
}

static float convert_internal_temperature(thermocouple_t* thermocouple) {
    int16_t internal_temp = ((int16_t)thermocouple->register_data & TC_INTERNAL_TEMP_MASK) >> TC_INTERNAL_TEMP_OFFSET;
    return (float)internal_temp * 0.0625f;
}

static thermocouple_flags_t get_thermocouple_flags(thermocouple_t* thermocouple) {
    thermocouple_flags_t flags;
    flags.OC = ((uint32_t)thermocouple->register_data >> TC_OC_BIT) & 0x01;
    flags.SCG = ((uint32_t)thermocouple->register_data >> TC_SCG_BIT) & 0x01;
    flags.SCV = ((uint32_t)thermocouple->register_data >> TC_SCV_BIT) & 0x01;
    flags.fault = ((uint32_t)thermocouple->register_data >> TC_FAULT_BIT) & 0x03;
    return flags;
}

esp_err_t thermocouple_read(thermocouple_t* thermocouple, float* temperature) {

    esp_err_t ret;

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 32;
    t.rx_buffer = thermocouple->register_data;
    ret = spi_device_transmit(thermocouple->spi_handle, &t);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read from SPI device: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Read data from SPI device: %08x",
             thermocouple->register_data);   

    *temperature = convert_thermocouple_temperature(thermocouple);

    float internal_temperature = convert_internal_temperature(thermocouple);

    ESP_LOGI(TAG, "Internal temperature: %.2f", internal_temperature);

    thermocouple_flags_t flags = get_thermocouple_flags(thermocouple);
    if(flags.fault) {
        ESP_LOGW(TAG, "Thermocouple fault detected: OC=%d, SCG=%d, SCV=%d",
                 flags.OC, flags.SCG, flags.SCV);
    }

    return ESP_OK;
}