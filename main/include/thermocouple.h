#pragma once

#include "esp_err.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/spi_common.h"

typedef struct {
    uint8_t OC : 1;
    uint8_t SCG : 1;
    uint8_t SCV : 1;
    uint8_t fault : 1;
} thermocouple_flags_t;

typedef struct {
    uint8_t id;
    uint8_t register_data[4];
    spi_device_handle_t spi_handle;
} thermocouple_t;

typedef struct {

    thermocouple_t thermocouples[3];

} thermocouple_manager_t;

esp_err_t thermocouple_manager_init(thermocouple_manager_t* manager);
esp_err_t thermocouple_init(thermocouple_t* thermocouple, uint8_t id);
esp_err_t thermocouple_read(thermocouple_t* thermocouple, float* temperature);