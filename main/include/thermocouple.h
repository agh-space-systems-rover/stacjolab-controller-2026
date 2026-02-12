#pragma once

#include "esp_err.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/spi_common.h"

#define TC_COUNT 3

#define TC_FILTER_SIZE 16

#define TC_0_ID 0x0
#define TC_1_ID 0x1
#define TC_2_ID 0x2

typedef struct {
    uint8_t OC : 1;
    uint8_t SCG : 1;
    uint8_t SCV : 1;
} thermocouple_flags_t;

typedef struct {

    uint8_t id;
    uint8_t register_data[4];
    spi_device_handle_t spi_handle;

    float filter_buffer[TC_FILTER_SIZE];
    size_t filter_newest_reading;
    float filtered_temperature;

    bool fault_detected;
    thermocouple_flags_t flags;

} thermocouple_t;

typedef struct {

    thermocouple_t thermocouples[3];

} thermocouple_manager_t;

esp_err_t thermocouple_manager_init(thermocouple_manager_t* manager);
thermocouple_t* get_thermocouple_by_id(uint8_t id);

esp_err_t thermocouple_init(thermocouple_t* thermocouple, uint8_t id);
esp_err_t thermocouple_read(thermocouple_t* thermocouple);
esp_err_t thermocouple_read_all(thermocouple_manager_t* manager);

bool get_thermocouple_fault_by_id(uint8_t id);

float get_filtered_temperature(thermocouple_t* thermocouple);
float get_raw_temperature(thermocouple_t* thermocouple);
float get_temperature_by_id(uint8_t id);