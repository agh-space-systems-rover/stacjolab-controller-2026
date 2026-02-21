//
// Created by lukasz on 01.05.2024.
// https://www.ti.com/lit/ds/symlink/ads122c04.pdf?ts=1714314941588&ref_url=https%253A%252F%252Fwww.ti.com%252Fdata-converters%252Fadc-circuit%252Fproducts.html

#ifndef ADC_EXTERNAL_DRIVER_H_
#define ADC_EXTERNAL_DRIVER_H_

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
 * @brief Type of the strain gauge beam used (each has slightly different calibration constant)
 */
typedef enum {
    EXTERNAL_ANALOG_BEAM_1 = 0,
    EXTERNAL_ANALOG_BEAM_2 = 1,
    EXTERNAL_ANALOG_BEAM_3 = 2,
    EXTERNAL_ANALOG_BEAM_4 = 3
} ExternalAnalog_StrainGaugeBeamTypeDef;

/**
 * @brief Initialization struct for the driver
 */
typedef struct {
    i2c_port_t i2c_port; // Fix: Use i2c_port_t directly
    ExternalAnalog_AddressPinTypeDef PinA0;
    ExternalAnalog_AddressPinTypeDef PinA1;
    ExternalAnalog_StrainGaugeBeamTypeDef BeamType;
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
    ExternalAnalog_StrainGaugeBeamTypeDef BeamType;

    int32_t VRef;
    uint8_t Gain;

    int32_t ADCValueFiltered;           // current filtered ADC value via moving average filter
    int32_t ADCValueCorrected;          // current filtered ADC value which is correctly offset to compensate tare weight and internal resistance
    int32_t ADCTareWeightValue;         // ADC value which should be interpreted as zero grams

    int32_t Voltage;                    // filtered voltage from (-VREF / GAIN) to ((+VREF / GAIN) - 1 LSB)
    int32_t Weight;                     // calculated weight in grams

    uint16_t CalibrationSampleIndex;
    int32_t* pCalibrationSamples;
    int32_t CalibrationOffset;

    int64_t MovingAverageSum;
    uint16_t MovingAverageSampleIndex;

    bool isModuleInitialized;
    bool isFilterInitialized;
    bool isCalibrating;
} ExternalAnalog_DriverTypeDef;

ExternalAnalog_StatusTypeDef ExternalAnalog_Driver_Init(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver, const ExternalAnalog_DriverInitTypeDef* pExternalAnalogInit);
ExternalAnalog_StatusTypeDef ExternalAnalog_Driver_DataReadyCallback(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver);
ExternalAnalog_StatusTypeDef ExternalAnalog_Driver_GetWeightValue(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver, int32_t* pData);
ExternalAnalog_StatusTypeDef ExternalAnalog_Driver_TareWeight(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver);

#endif //ADC_EXTERNAL_DRIVER_H_