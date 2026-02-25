
#include "adc_external_driver.h"
#include "adc_external_registers.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

static const char *TAG_ADC = "ADC_EXTERNAL_DRIVER";


/**
 * @brief Internal reference voltage value in uV
 */
#define VREF_INTERNAL_VALUE 2048000

/**
 * @brief External reference voltage value in uV-
 */
#define VREF_EXTERNAL_VALUE 2500000

/**
 * @brief Number of samples needed to produce one filtered value
 */
#ifdef CONFIG_TENSOMETER_MOVING_AVERAGE_SIZE
static uint32_t MovingAverageSize = CONFIG_TENSOMETER_MOVING_AVERAGE_SIZE;
#else
static uint32_t MovingAverageSize = 256;
#endif

void ExternalAnalog_Driver_SetMovingAverageSize(uint32_t size) {
    MovingAverageSize = size;
}




/**
 * @brief Check if function returns EXTERNAL_ANALOG_DRIVER_OK;
 * if not, power-down the module and return EXTERNAL_ANALOG_DRIVER_ERROR
 */
#define TRY_EXECUTE(fn, pDriver) {\
    if ((fn) != EXTERNAL_ANALOG_DRIVER_OK) {\
        command_powerdown((pDriver));\
        return EXTERNAL_ANALOG_DRIVER_ERROR;\
    }\
}

static int32_t convert_int24_to_int32(int32_t int24Value);
static uint8_t get_slave_i2c_address(ExternalAnalog_AddressPinTypeDef pinA0, ExternalAnalog_AddressPinTypeDef pinA1);
static uint8_t gain_to_number(ExternalAnalog_Gain gain);
// static uint32_t get_beam_calibration_constant(ExternalAnalog_StrainGaugeBeamTypeDef beamType);
static ExternalAnalog_StatusTypeDef send_command(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver, uint8_t command);
static ExternalAnalog_StatusTypeDef command_reset(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver);
static ExternalAnalog_StatusTypeDef command_start(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver);
static ExternalAnalog_StatusTypeDef command_powerdown(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver);
static ExternalAnalog_StatusTypeDef command_read_data(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver, uint32_t* pData);
static ExternalAnalog_StatusTypeDef command_register_read(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver, ExternalAnalog_RegisterAddress address, uint8_t* pData);
static ExternalAnalog_StatusTypeDef command_register_write(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver, ExternalAnalog_RegisterAddress address, const uint8_t* pData);



/**
 * @brief Init the external analog module driver - write register config, calibrate and start
 *
 * @param pExternalAnalogDriver: pointer to the struct being initialized
 * @param pExternalAnalogInit: pointer to the struct with config
 *
 * @retval ExternalAnalog_StatusTypeDef: information if the module has beed initialized correctly
 */
ExternalAnalog_StatusTypeDef ExternalAnalog_Driver_Init(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver, const ExternalAnalog_DriverInitTypeDef* pExternalAnalogInit) {
    if (!pExternalAnalogDriver || !pExternalAnalogInit) {
        ESP_LOGE(TAG_ADC, "Invalid arguments for ADC initialization");
        return EXTERNAL_ANALOG_DRIVER_ERROR;
    }

    // Set the I2C port and calculate the device address
    pExternalAnalogDriver->i2c_port = pExternalAnalogInit->i2c_port;
    pExternalAnalogDriver->DeviceAddress = get_slave_i2c_address(pExternalAnalogInit->PinA0, pExternalAnalogInit->PinA1);

    // Initialize other fields
    pExternalAnalogDriver->Mux = pExternalAnalogInit->Mux;
    pExternalAnalogDriver->GainEnum = pExternalAnalogInit->Gain; 
    pExternalAnalogDriver->DataRateEnum = pExternalAnalogInit->DataRate;
    pExternalAnalogDriver->DataReadyCallback = pExternalAnalogInit->DataReadyCallback;
    pExternalAnalogDriver->ADCValueFiltered = 0;

    // pExternalAnalogDriver->ADCValueCorrected = 0; // Removed member
    pExternalAnalogDriver->VoltageOffset = 0;
    pExternalAnalogDriver->Voltage = 0;
    pExternalAnalogDriver->IsDataValid = false;
    pExternalAnalogDriver->isModuleInitialized = false;

    pExternalAnalogDriver->MovingAverageSum = 0;
    pExternalAnalogDriver->MovingAverageSampleIndex = 0;
    vTaskDelay(pdMS_TO_TICKS(5)); // Allow the ADC to stabilize

    // Reset the ADC module (Note: Resetting might affect other channels if sharing the device)
    // For single device with multiple channels, re-initializing resets it. 
    if (command_reset(pExternalAnalogDriver) != EXTERNAL_ANALOG_DRIVER_OK) {
        ESP_LOGE(TAG_ADC, "Failed to reset ADC module");
        return EXTERNAL_ANALOG_DRIVER_ERROR;
    }

    // Wait for the reset to complete as per datasheet (min 50us, using 10ms for safety)
    vTaskDelay(pdMS_TO_TICKS(10));

    // Configure the ADC registers
    ExternalAnalog_Register0 register0 = { .mux = pExternalAnalogDriver->Mux, .gain = pExternalAnalogInit->Gain };
    ExternalAnalog_Register1 register1 = { .cm = CONTINUOUS_MODE, .vref = VREF_EXTERNAL, .mode = NORMAL_MODE, .dr = pExternalAnalogInit->DataRate };

    pExternalAnalogDriver->VRef = (register1.vref == VREF_INTERNAL) ? VREF_INTERNAL_VALUE : VREF_EXTERNAL_VALUE;
    pExternalAnalogDriver->Gain = gain_to_number(register0.gain);

    if (command_register_write(pExternalAnalogDriver, REGISTER0, (uint8_t*)&register0) != EXTERNAL_ANALOG_DRIVER_OK ||
        command_register_write(pExternalAnalogDriver, REGISTER1, (uint8_t*)&register1) != EXTERNAL_ANALOG_DRIVER_OK) {
        ESP_LOGE(TAG_ADC, "Failed to configure ADC registers");
        return EXTERNAL_ANALOG_DRIVER_ERROR;
    }
    
    // Start the ADC
    if (command_start(pExternalAnalogDriver) != EXTERNAL_ANALOG_DRIVER_OK) {
        ESP_LOGE(TAG_ADC, "Failed to start ADC");
        return EXTERNAL_ANALOG_DRIVER_ERROR;
    }

    pExternalAnalogDriver->isModuleInitialized = true;
    ESP_LOGI(TAG_ADC, "ADC initialized successfully");
    return EXTERNAL_ANALOG_DRIVER_OK;
}

/**
 * @brief Callback function called when the external nDRDY GPIO pins changes state from HIGH to LOW
 *
 * @param pExternalAnalogDriver: pointer to the driver struct
 *
 * @retval ExternalAnalog_StatusTypeDef: information if there were any errors
 */

ExternalAnalog_StatusTypeDef ExternalAnalog_Driver_DataReadyCallback(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver) {
    if (!pExternalAnalogDriver->isModuleInitialized) {
        ESP_LOGE(TAG_ADC, "ADC module not initialized");
        return EXTERNAL_ANALOG_DRIVER_ERROR;
    }

    // 1. Configure MUX (Switch channel)
    ExternalAnalog_Register0 register0 = { .mux = pExternalAnalogDriver->Mux, .gain = pExternalAnalogDriver->GainEnum, .pga_bypass = 0 }; // Use configured gain
    if (command_register_write(pExternalAnalogDriver, REGISTER0, (uint8_t*)&register0) != EXTERNAL_ANALOG_DRIVER_OK) {
        ESP_LOGE(TAG_ADC, "Failed to write register 0");
        return EXTERNAL_ANALOG_DRIVER_ERROR; 
    }

    // 2. Start Conversion
    if (command_start(pExternalAnalogDriver) != EXTERNAL_ANALOG_DRIVER_OK) {
         ESP_LOGE(TAG_ADC, "Failed to start ADC conversion");   
         return EXTERNAL_ANALOG_DRIVER_ERROR;
    }
    
    // 3. Wait for conversion (Single shot mode) based on Data Rate
    uint32_t conversion_time_ms = 55; // Default for 20 SPS (50ms + margin)
    
    switch(pExternalAnalogDriver->DataRateEnum) {
        case DATA_RATE_20_SPS:   conversion_time_ms = 55; break; // 50ms
        case DATA_RATE_45_SPS:   conversion_time_ms = 25; break; // 22.2ms
        case DATA_RATE_90_SPS:   conversion_time_ms = 15; break; // 11.1ms
        case DATA_RATE_175_SPS:  conversion_time_ms = 8;  break; // 5.7ms
        case DATA_RATE_330_SPS:  conversion_time_ms = 5;  break; // 3ms
        case DATA_RATE_600_SPS:  conversion_time_ms = 3;  break; // 1.6ms
        case DATA_RATE_1000_SPS: conversion_time_ms = 2;  break; // 1ms
        default:                 conversion_time_ms = 55; break;
    }

    // Ensure at least 2 ticks wait if the time is very short but non-zero
    // (FreeRTOS tick rate is usually 100Hz or 1000Hz, relying on 1ms delays might be tricky if tick is 10ms)
    TickType_t delay_ticks = pdMS_TO_TICKS(conversion_time_ms);
    if(delay_ticks < 2) delay_ticks = 2; // Minimum 2 ticks to be safe with scheduler jitter
    
    vTaskDelay(delay_ticks);

    // 4. Read Data
    uint32_t adc_value_24;
    if (command_read_data(pExternalAnalogDriver, &adc_value_24) != EXTERNAL_ANALOG_DRIVER_OK) {
        return EXTERNAL_ANALOG_DRIVER_ERROR;
    }

    const int32_t adc_value = convert_int24_to_int32(adc_value_24);
    
    // ESP_LOGI(TAG_ADC, "Raw ADC value: %ld", adc_value);
    // 5. Process Data
    // User wants "reading microvolts".
    pExternalAnalogDriver->MovingAverageSum += adc_value;
    pExternalAnalogDriver->MovingAverageSampleIndex++;

    if (pExternalAnalogDriver->MovingAverageSampleIndex >= MovingAverageSize) {
        pExternalAnalogDriver->ADCValueFiltered = pExternalAnalogDriver->MovingAverageSum / (int32_t)MovingAverageSize;
        pExternalAnalogDriver->MovingAverageSum = 0;
        pExternalAnalogDriver->MovingAverageSampleIndex = 0;

        // Voltage Calculation (V_in = (Code * V_ref) / (Gain * 2^23))
        // VRef is ~2.048V or 2.5V (uV).
        // Result in uV.
        // Using int64 to prevent overflow.
        // Voltage = (ADC * VRef) / (Gain * 8388608)
        int64_t v_ref_uv = pExternalAnalogDriver->VRef;
        int64_t gain = pExternalAnalogDriver->Gain;
        
        // Raw voltage
        // Note: 8388608 is 2^23
        int32_t raw_voltage = (int32_t)((((int64_t)pExternalAnalogDriver->ADCValueFiltered * v_ref_uv) / gain) / 8388608);
        pExternalAnalogDriver->Voltage = raw_voltage - pExternalAnalogDriver->VoltageOffset;

        pExternalAnalogDriver->IsDataValid = true;

        // Invoke callback
        if(pExternalAnalogDriver->DataReadyCallback) {
            pExternalAnalogDriver->DataReadyCallback(pExternalAnalogDriver->Voltage);
        }
    }

    return EXTERNAL_ANALOG_DRIVER_OK;
}

/**
 * @brief Tare the current voltage reading to 0
 *
 * @param pExternalAnalogDriver: pointer to the driver struct
 *
 * @retval ExternalAnalog_StatusTypeDef: information if there were any errors
 */
ExternalAnalog_StatusTypeDef ExternalAnalog_Driver_Tare(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver) {
    if (!pExternalAnalogDriver->IsDataValid) {
        ESP_LOGE(TAG_ADC, "Measurement not ready or invalid");
        return EXTERNAL_ANALOG_DRIVER_ERROR;
    }
    
    // Calculate current raw voltage (we add back the old offset)
    int32_t current_raw_voltage = pExternalAnalogDriver->Voltage + pExternalAnalogDriver->VoltageOffset;
    pExternalAnalogDriver->VoltageOffset = current_raw_voltage;
    
    ESP_LOGI(TAG_ADC, "Tared. New offset: %ld uV", pExternalAnalogDriver->VoltageOffset);
    return EXTERNAL_ANALOG_DRIVER_OK;
}

/**
 * @brief Subtract current tare weight from measurements
 *
 * @param pExternalAnalogDriver: pointer to the driver struct
 *
 * @retval ExternalAnalog_StatusTypeDef: information if there were any errors
 */
// ExternalAnalog_StatusTypeDef ExternalAnalog_Driver_TareWeight(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver) {
//     // Deprecated or removed functionality
//     return EXTERNAL_ANALOG_DRIVER_OK;
// }

// ExternalAnalog_StatusTypeDef ExternalAnalog_Driver_SetScale(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver, float scale) {
//     // Deprecated or removed functionality
//     return EXTERNAL_ANALOG_DRIVER_OK;
// }

// float ExternalAnalog_Driver_GetScale(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver) {
//     // Deprecated or removed functionality
//     return 1.0f;
// }

/**
 * @brief Convert signed 24-bit integer to signed 32-bit integer
 *
 * @param int24_value: 24-bit integer value
 *
 * @retval int32_t: converted value
 */
static int32_t convert_int24_to_int32(int32_t int24Value) {
    int24Value &= 0x00FFFFFF;

    const uint8_t bits = 24;
    const uint32_t mask = 1U << (bits - 1);

    return (int24Value ^ mask) - mask;
}

/**
 * @brief Get the module I2C address
 *
 * @param pinA0: pin A0 electrical connection
 * @param pinA1: pin A1 electrical connection
 *
 * @retval uint8_t: 7-bit module address
 */
static uint8_t get_slave_i2c_address(ExternalAnalog_AddressPinTypeDef pinA0, ExternalAnalog_AddressPinTypeDef pinA1) {
    // according to the logic table provided in the docs on page 35
    return I2C_SLAVE_BASE_ADDRESS + pinA0 + (pinA1 << 2);
}

/**
 * @brief Convert gain as enum to a numeric value
 *
 * @param gain: gain value as enum
 *
 * @retval uint8_t: numeric gain value
 */
static uint8_t gain_to_number(ExternalAnalog_Gain gain) {
    return (1U << gain);
}

/**
 * @brief Send a generic command to the module
 *
 * @param pExternalAnalogDriver: pointer to the driver struct
 * @param command: 8-bit command to send
 *
 * @retval ExternalAnalog_StatusTypeDef: information if there were any errors
 */
static ExternalAnalog_StatusTypeDef send_command(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver, uint8_t command) {
    esp_err_t ret = i2c_master_write_to_device(pExternalAnalogDriver->i2c_port, 
                                               pExternalAnalogDriver->DeviceAddress, 
                                               &command, 1, 100 / portTICK_PERIOD_MS);
    return (ret == ESP_OK) ? EXTERNAL_ANALOG_DRIVER_OK : EXTERNAL_ANALOG_DRIVER_ERROR;
}

/**
 * @brief Reset the module
 *
 * @param pExternalAnalogDriver: pointer to the driver struct
 *
 * @retval ExternalAnalog_StatusTypeDef: information if there were any errors
 */
static ExternalAnalog_StatusTypeDef command_reset(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver) {
    return send_command(pExternalAnalogDriver, COMMAND_RESET);
}

/**
 * @brief Start the module
 *
 * @param pExternalAnalogDriver: pointer to the driver struct
 *
 * @retval ExternalAnalog_StatusTypeDef: information if there were any errors
 */
static ExternalAnalog_StatusTypeDef command_start(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver) {
    return send_command(pExternalAnalogDriver, COMMAND_START_SYNC);
}

/**
 * @brief Put the module into power-down mode
 *
 * @param pExternalAnalogDriver: pointer to the driver struct
 *
 * @retval ExternalAnalog_StatusTypeDef: information if there were any errors
 */
static __attribute__((unused)) ExternalAnalog_StatusTypeDef command_powerdown(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver) {
    return send_command(pExternalAnalogDriver, COMMAND_POWERDOWN);
}

/**
 * @brief Read 3 bytes of converted ADC data
 *
 * @param pExternalAnalogDriver: pointer to the driver struct
 * @param pData: pointer to where save the data
 *
 * @retval ExternalAnalog_StatusTypeDef: information if there were any errors
 */
static ExternalAnalog_StatusTypeDef command_read_data(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver, uint32_t* pData) {
    uint8_t data[CONVERTED_DATA_SIZE] = {0};
    esp_err_t ret = i2c_master_write_read_device(pExternalAnalogDriver->i2c_port, 
                                                 pExternalAnalogDriver->DeviceAddress, 
                                                 (uint8_t[]){COMMAND_RDATA}, 1,  // Fix: Remove '&' from COMMAND_RDATA
                                                 data, sizeof(data), 
                                                 100 / portTICK_PERIOD_MS);
    if (ret == ESP_OK) {
        *pData = (data[0] << 16) + (data[1] << 8) + data[2];
        pExternalAnalogDriver->IsDataValid = true;
        return EXTERNAL_ANALOG_DRIVER_OK;
    }
    pExternalAnalogDriver->IsDataValid = false;
    return EXTERNAL_ANALOG_DRIVER_ERROR;
}

static __attribute__((unused)) ExternalAnalog_StatusTypeDef command_register_read(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver, ExternalAnalog_RegisterAddress address, uint8_t* pData) {
    uint8_t command_rreg = COMMAND_RREG_BASE + (address << 2);
    esp_err_t ret = i2c_master_write_read_device(pExternalAnalogDriver->i2c_port, 
                                                 pExternalAnalogDriver->DeviceAddress, 
                                                 &command_rreg, 1, pData, 1, 
                                                 100 / portTICK_PERIOD_MS);
    return (ret == ESP_OK) ? EXTERNAL_ANALOG_DRIVER_OK : EXTERNAL_ANALOG_DRIVER_ERROR;
}

// Replace HAL_I2C_Mem_Write with i2c_master_write_to_device
static ExternalAnalog_StatusTypeDef command_register_write(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver, ExternalAnalog_RegisterAddress address, const uint8_t* pData) {
    uint8_t command_wreg = COMMAND_WREG_BASE + (address << 2);
    uint8_t data[2] = {command_wreg, *pData};
    esp_err_t ret = i2c_master_write_to_device(pExternalAnalogDriver->i2c_port, 
                                               pExternalAnalogDriver->DeviceAddress, 
                                               data, sizeof(data), 
                                               100 / portTICK_PERIOD_MS);
    return (ret == ESP_OK) ? EXTERNAL_ANALOG_DRIVER_OK : EXTERNAL_ANALOG_DRIVER_ERROR;
}

