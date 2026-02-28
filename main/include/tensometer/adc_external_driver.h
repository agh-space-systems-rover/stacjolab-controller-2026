//
// Created by lukasz on 01.05.2024.
// https://www.ti.com/lit/ds/symlink/ads122c04.pdf?ts=1714314941588&ref_url=https%253A%252F%252Fwww.ti.com%252Fdata-converters%252Fadc-circuit%252Fproducts.html

#ifndef ADC_EXTERNAL_DRIVER_H_
#define ADC_EXTERNAL_DRIVER_H_

#include "adc_external_registers.h"

#include "driver/i2c.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Return type of External Analog functions, used for basic error handling
 */
typedef enum {
    EXTERNAL_ANALOG_DRIVER_OK,
    EXTERNAL_ANALOG_DRIVER_ERROR
} ExternalAnalog_StatusTypeDef;

/**
 * @brief Type of pins that A0 and A1 could be connected to (determines I2C slave address)
 */
typedef enum {
    EXTERNAL_ANALOG_PIN_DGND = 0,
    EXTERNAL_ANALOG_PIN_DVDD = 1,
    EXTERNAL_ANALOG_PIN_SDA = 2,
    EXTERNAL_ANALOG_PIN_SCL = 3
} ExternalAnalog_AddressPinTypeDef;

/**
 * @brief Initialization struct for the driver
 */
typedef struct {
    i2c_port_t i2c_port; // Fix: Use i2c_port_t directly
    ExternalAnalog_AddressPinTypeDef PinA0;
    ExternalAnalog_AddressPinTypeDef PinA1;
    ExternalAnalog_Mux Mux;
    ExternalAnalog_Gain Gain;
    ExternalAnalog_DataRate DataRate;
    void (*DataReadyCallback)(int32_t value);
} ExternalAnalog_DriverInitTypeDef;

/**
 * @brief Struct holding the External Analog driver data
 */
typedef struct {
    i2c_port_t i2c_port;
    uint8_t DeviceAddress;
    bool IsDataValid;

    ExternalAnalog_AddressPinTypeDef PinA0;
    ExternalAnalog_AddressPinTypeDef PinA1;
    ExternalAnalog_Mux Mux;
    ExternalAnalog_Gain GainEnum;
    ExternalAnalog_DataRate DataRateEnum;
    void (*DataReadyCallback)(int32_t value);
    
    int32_t VRef;
    uint8_t Gain;

    int32_t ADCValueFiltered;           // current filtered ADC value via moving average filter
    int32_t Voltage;                    // filtered voltage in microvolts
    int32_t VoltageOffset;              // voltage offset for tare
    
    int64_t MovingAverageSum;
    uint32_t MovingAverageSampleIndex; // Changed to uint32_t to match size

    bool isModuleInitialized;
} ExternalAnalog_DriverTypeDef;

ExternalAnalog_StatusTypeDef ExternalAnalog_Driver_Init(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver, const ExternalAnalog_DriverInitTypeDef* pExternalAnalogInit);
ExternalAnalog_StatusTypeDef ExternalAnalog_Driver_DataReadyCallback(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver);
ExternalAnalog_StatusTypeDef ExternalAnalog_Driver_Tare(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver);
void ExternalAnalog_Driver_SetMovingAverageSize(uint32_t size);

#endif //ADC_EXTERNAL_DRIVER_H_