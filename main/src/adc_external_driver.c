/*
 * ExternalAnalog_Driver.c
 *
 *  Created on: Jul 29, 2024
 *      Author: Jakub Halfar
 */

#include "adc_external_driver.h"
#include "adc_external_registers.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_now_driver.h"

#include <string.h>

static const char *TAG_ADC = "ADC_EXTERNAL_DRIVER";

/**
 * @brief Enable live expressions debug
 */
#define EXTERNAL_ANALOG_DEBUG

#ifdef EXTERNAL_ANALOG_DEBUG
    /**
     * @brief I2C slave used to show data in debug mode
     */
    #define EXTERNAL_ANALOG_DEBUG_TENSO_1_DEVICE_ID 64
    #define EXTERNAL_ANALOG_DEBUG_TENSO_2_DEVICE_ID 65
#endif

/**
 * @brief Max I2C transmit/receive time in polling mode
 */
#define I2C_DELAY_TIME 100

/**
 * @brief Internal reference voltage value in uV
 */
#define VREF_INTERNAL_VALUE 2048000

/**
 * @brief External reference voltage value in uV-
 */
#define VREF_EXTERNAL_VALUE 2500000

/**
 * @brief Number of samples to take to calibrate the module
 */
#define CALIBRATION_SAMPLES_AMOUNT 512

/**
 * @brief Max time in ms to wait to finish the calibration process
 */
#define CALIBRATION_MAX_DELAY 10000

/**
 * @brief Numerator of the slope coefficient in units: grams per (ADC corrected value per gain)
 */
#define CALIBRATION_RATIO_NUMERATOR_BEAM_1 1277354081
#define CALIBRATION_RATIO_NUMERATOR_BEAM_2 1296107563
#define CALIBRATION_RATIO_NUMERATOR_BEAM_3 1265585114
#define CALIBRATION_RATIO_NUMERATOR_BEAM_4 1321518968

/**
 * @brief Denominator of the slope coefficient in units: grams per (ADC corrected value per gain)
 */
#define CALIBRATION_RATIO_DENOMINATOR 1000000000

/**
 * @brief Number of samples needed to produce one filtered value
 */
#define MOVING_AVERAGE_SIZE 256



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
static uint32_t get_beam_calibration_constant(ExternalAnalog_StrainGaugeBeamTypeDef beamType);
static ExternalAnalog_StatusTypeDef send_command(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver, uint8_t command);
static ExternalAnalog_StatusTypeDef command_reset(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver);
static ExternalAnalog_StatusTypeDef command_start(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver);
static ExternalAnalog_StatusTypeDef command_powerdown(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver);
static ExternalAnalog_StatusTypeDef command_read_data(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver, uint32_t* pData);
static ExternalAnalog_StatusTypeDef command_register_read(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver, ExternalAnalog_RegisterAddress address, uint8_t* pData);
static ExternalAnalog_StatusTypeDef command_register_write(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver, ExternalAnalog_RegisterAddress address, const uint8_t* pData);
// Commented out to suppress unused function warning
/*
static ExternalAnalog_StatusTypeDef sanity_check_registers(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver, ExternalAnalog_Register0* pReg0, ExternalAnalog_Register1* pReg1, ExternalAnalog_Register2* pReg2, ExternalAnalog_Register3* pReg3) {
    uint8_t data;
    ExternalAnalog_StatusTypeDef status;
    void* registers[] = { pReg0, pReg1, pReg2, pReg3 };
    ExternalAnalog_RegisterAddress addresses[] = { REGISTER0, REGISTER1, REGISTER2, REGISTER3 };

    for (uint8_t i = 0; i < 3; i++) {
        status = command_register_read(pExternalAnalogDriver, addresses[i], &data);
        if (memcmp(&data, registers[i], 1) != 0 || status != EXTERNAL_ANALOG_DRIVER_OK)
            return EXTERNAL_ANALOG_DRIVER_ERROR;
    }

    return EXTERNAL_ANALOG_DRIVER_OK;
}
*/
static ExternalAnalog_StatusTypeDef calibrate(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver);

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
    pExternalAnalogDriver->BeamType = pExternalAnalogInit->BeamType;
    pExternalAnalogDriver->ADCValueFiltered = 0;
    pExternalAnalogDriver->ADCValueCorrected = 0;
    pExternalAnalogDriver->ADCTareWeightValue = 0;
    pExternalAnalogDriver->Weight = 0;
    pExternalAnalogDriver->isModuleInitialized = false;

    vTaskDelay(pdMS_TO_TICKS(5)); // Allow the ADC to stabilize

    // Reset the ADC module
    if (command_reset(pExternalAnalogDriver) != EXTERNAL_ANALOG_DRIVER_OK) {
        ESP_LOGE(TAG_ADC, "Failed to reset ADC module");
        return EXTERNAL_ANALOG_DRIVER_ERROR;
    }

    // Configure the ADC registers
    ExternalAnalog_Register0 register0 = { .mux = MUX_AIN_PN_12, .gain = GAIN64 };
    ExternalAnalog_Register1 register1 = { .cm = CONTINUOUS_MODE, .vref = VREF_EXTERNAL, .mode = NORMAL_MODE, .dr = DATA_RATE_175_SPS };

    pExternalAnalogDriver->VRef = (register1.vref == VREF_INTERNAL) ? VREF_INTERNAL_VALUE : VREF_EXTERNAL_VALUE;
    pExternalAnalogDriver->Gain = gain_to_number(register0.gain);

    if (command_register_write(pExternalAnalogDriver, REGISTER0, (uint8_t*)&register0) != EXTERNAL_ANALOG_DRIVER_OK ||
        command_register_write(pExternalAnalogDriver, REGISTER1, (uint8_t*)&register1) != EXTERNAL_ANALOG_DRIVER_OK) {
        ESP_LOGE(TAG_ADC, "Failed to configure ADC registers");
        return EXTERNAL_ANALOG_DRIVER_ERROR;
    }

    // Calibrate the ADC
    if (calibrate(pExternalAnalogDriver) != EXTERNAL_ANALOG_DRIVER_OK) {
        ESP_LOGE(TAG_ADC, "Failed to calibrate ADC");
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

#ifdef EXTERNAL_ANALOG_DEBUG
    static uint32_t l_adc_value_24_1;
    static int32_t l_adc_value_1;
    static int32_t l_adc_value_filtered_1;
    static int32_t l_adc_value_corrected_1;
    static int32_t l_voltage_1;
    static int32_t l_weight_1;

    static uint32_t l_adc_value_24_2;
    static int32_t l_adc_value_2;
    static int32_t l_adc_value_filtered_2;
    static int32_t l_adc_value_corrected_2;
    static int32_t l_voltage_2;
    static int32_t l_weight_2;
#endif

ExternalAnalog_StatusTypeDef ExternalAnalog_Driver_DataReadyCallback(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver) {
    if (!pExternalAnalogDriver->isModuleInitialized) {
        ESP_LOGE(TAG_ADC, "ADC module not initialized");
        return EXTERNAL_ANALOG_DRIVER_ERROR;
    }

    uint32_t adc_value_24;
    if (command_read_data(pExternalAnalogDriver, &adc_value_24) != EXTERNAL_ANALOG_DRIVER_OK) {
        ESP_LOGE(TAG_ADC, "Failed to read ADC data");
        return EXTERNAL_ANALOG_DRIVER_ERROR;
    }

    const int32_t adc_value = convert_int24_to_int32(adc_value_24);

#ifdef EXTERNAL_ANALOG_DEBUG
    if (pExternalAnalogDriver->DeviceAddress == EXTERNAL_ANALOG_DEBUG_TENSO_1_DEVICE_ID) {
        l_adc_value_24_1 = adc_value_24;
        l_adc_value_1 = adc_value;
    }
    if (pExternalAnalogDriver->DeviceAddress == EXTERNAL_ANALOG_DEBUG_TENSO_2_DEVICE_ID) {
        l_adc_value_24_2 = adc_value_24;
        l_adc_value_2 = adc_value;
    }
#endif

    if (pExternalAnalogDriver->isCalibrating) {
        if (pExternalAnalogDriver->CalibrationSampleIndex < CALIBRATION_SAMPLES_AMOUNT && pExternalAnalogDriver->pCalibrationSamples) {
            pExternalAnalogDriver->pCalibrationSamples[pExternalAnalogDriver->CalibrationSampleIndex++] = adc_value;
        }
    } else {
        pExternalAnalogDriver->MovingAverageSum += adc_value;
        pExternalAnalogDriver->MovingAverageSampleIndex++;

        if (pExternalAnalogDriver->MovingAverageSampleIndex >= MOVING_AVERAGE_SIZE) {
            pExternalAnalogDriver->ADCValueFiltered = pExternalAnalogDriver->MovingAverageSum / MOVING_AVERAGE_SIZE;
            pExternalAnalogDriver->MovingAverageSampleIndex = 0;
            pExternalAnalogDriver->MovingAverageSum = 0;

            const int32_t adc_value_corrected = pExternalAnalogDriver->ADCValueFiltered - pExternalAnalogDriver->CalibrationOffset - pExternalAnalogDriver->ADCTareWeightValue;
            pExternalAnalogDriver->ADCValueCorrected = adc_value_corrected;

            const int32_t voltage = (((int64_t)adc_value_corrected * pExternalAnalogDriver->VRef) / ((int16_t)pExternalAnalogDriver->Gain)) >> 23;
            pExternalAnalogDriver->Voltage = voltage;

            const int32_t weight = ((int64_t)pExternalAnalogDriver->ADCValueCorrected * get_beam_calibration_constant(pExternalAnalogDriver->BeamType)) / ((int64_t)pExternalAnalogDriver->Gain * CALIBRATION_RATIO_DENOMINATOR);
            pExternalAnalogDriver->Weight = weight;

            pExternalAnalogDriver->isFilterInitialized = true;
            pExternalAnalogDriver->IsDataValid = true;

            ESP_LOGI(TAG_ADC, "Measurement updated: Voltage=%ld uV, Weight=%ld grams", voltage, weight);

#ifdef EXTERNAL_ANALOG_DEBUG
            if (pExternalAnalogDriver->DeviceAddress == EXTERNAL_ANALOG_DEBUG_TENSO_1_DEVICE_ID) {
                l_adc_value_filtered_1 = pExternalAnalogDriver->ADCValueFiltered;
                l_adc_value_corrected_1 = pExternalAnalogDriver->ADCValueCorrected;
                l_voltage_1 = pExternalAnalogDriver->Voltage;
                l_weight_1 = pExternalAnalogDriver->Weight;
            }
            if (pExternalAnalogDriver->DeviceAddress == EXTERNAL_ANALOG_DEBUG_TENSO_2_DEVICE_ID) {
                l_adc_value_filtered_2 = pExternalAnalogDriver->ADCValueFiltered;
                l_adc_value_corrected_2 = pExternalAnalogDriver->ADCValueCorrected;
                l_voltage_2 = pExternalAnalogDriver->Voltage;
                l_weight_2 = pExternalAnalogDriver->Weight;
            }
#endif
        }
    }

    return EXTERNAL_ANALOG_DRIVER_OK;
}

/**
 * @brief Get the weight value in grams (possibly negative)
 *
 * @param pExternalAnalogDriver: pointer to the driver struct
 * @param pData: pointer to where save the weight value
 *
 * @retval ExternalAnalog_StatusTypeDef: information if there were any errors
 */
ExternalAnalog_StatusTypeDef ExternalAnalog_Driver_GetWeightValue(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver, int32_t* pData) {
    if (!pExternalAnalogDriver->IsDataValid || !pExternalAnalogDriver->isFilterInitialized || pExternalAnalogDriver->isCalibrating) {
        ESP_LOGE(TAG_ADC, "Measurement not ready or invalid");
        return EXTERNAL_ANALOG_DRIVER_ERROR;
    }

    *pData = pExternalAnalogDriver->Weight;
    ESP_LOGI(TAG_ADC, "Weight value retrieved: %ld grams", *pData);
    return EXTERNAL_ANALOG_DRIVER_OK;
}

/**
 * @brief Subtract current tare weight from measurements
 *
 * @param pExternalAnalogDriver: pointer to the driver struct
 *
 * @retval ExternalAnalog_StatusTypeDef: information if there were any errors
 */
ExternalAnalog_StatusTypeDef ExternalAnalog_Driver_TareWeight(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver) {
    if (!pExternalAnalogDriver->IsDataValid || !pExternalAnalogDriver->isFilterInitialized || pExternalAnalogDriver->isCalibrating)
        return EXTERNAL_ANALOG_DRIVER_ERROR;

    pExternalAnalogDriver->ADCTareWeightValue = pExternalAnalogDriver->ADCValueFiltered - pExternalAnalogDriver->CalibrationOffset;

    return EXTERNAL_ANALOG_DRIVER_OK;
}

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
 * @brief Get the numerator of the calibration ratio constant for the specified beam
 *
 * @param ExternalAnalog_StrainGaugeBeamTypeDef: beam type enum
 *
 * @retval uint32_t: numerator of the beam constant
 */
static uint32_t get_beam_calibration_constant(ExternalAnalog_StrainGaugeBeamTypeDef beamType) {
    switch (beamType) {
    case EXTERNAL_ANALOG_BEAM_1:
        return CALIBRATION_RATIO_NUMERATOR_BEAM_1;
    case EXTERNAL_ANALOG_BEAM_2:
        return CALIBRATION_RATIO_NUMERATOR_BEAM_2;
    case EXTERNAL_ANALOG_BEAM_3:
        return CALIBRATION_RATIO_NUMERATOR_BEAM_3;
    case EXTERNAL_ANALOG_BEAM_4:
        return CALIBRATION_RATIO_NUMERATOR_BEAM_4;
    default:
        return CALIBRATION_RATIO_NUMERATOR_BEAM_1;
    }
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
                                               &command, 1, pdMS_TO_TICKS(I2C_DELAY_TIME));
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
static ExternalAnalog_StatusTypeDef command_powerdown(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver) {
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
                                                 pdMS_TO_TICKS(I2C_DELAY_TIME));
    if (ret == ESP_OK) {
        *pData = (data[0] << 16) + (data[1] << 8) + data[2];
        pExternalAnalogDriver->IsDataValid = true;
        return EXTERNAL_ANALOG_DRIVER_OK;
    }
    pExternalAnalogDriver->IsDataValid = false;
    return EXTERNAL_ANALOG_DRIVER_ERROR;
}

// Replace HAL_I2C_Mem_Read with i2c_master_write_read_device
static ExternalAnalog_StatusTypeDef command_register_read(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver, ExternalAnalog_RegisterAddress address, uint8_t* pData) {
    uint8_t command_rreg = COMMAND_RREG_BASE + (address << 2);
    esp_err_t ret = i2c_master_write_read_device(pExternalAnalogDriver->i2c_port, 
                                                 pExternalAnalogDriver->DeviceAddress, 
                                                 &command_rreg, 1, pData, 1, 
                                                 pdMS_TO_TICKS(I2C_DELAY_TIME));
    return (ret == ESP_OK) ? EXTERNAL_ANALOG_DRIVER_OK : EXTERNAL_ANALOG_DRIVER_ERROR;
}

// Replace HAL_I2C_Mem_Write with i2c_master_write_to_device
static ExternalAnalog_StatusTypeDef command_register_write(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver, ExternalAnalog_RegisterAddress address, const uint8_t* pData) {
    uint8_t command_wreg = COMMAND_WREG_BASE + (address << 2);
    uint8_t data[2] = {command_wreg, *pData};
    esp_err_t ret = i2c_master_write_to_device(pExternalAnalogDriver->i2c_port, 
                                               pExternalAnalogDriver->DeviceAddress, 
                                               data, sizeof(data), 
                                               pdMS_TO_TICKS(I2C_DELAY_TIME));
    return (ret == ESP_OK) ? EXTERNAL_ANALOG_DRIVER_OK : EXTERNAL_ANALOG_DRIVER_ERROR;
}

/**
 * @brief Check if user register data was saved correctly on the module
 *
 * @param pExternalAnalogDriver: pointer to the driver struct
 * @param pReg0: pointer to the register 0 struct
 * @param pReg1: pointer to the register 1 struct
 * @param pReg2: pointer to the register 2 struct
 * @param pReg3: pointer to the register 3 struct
 *
 * @retval ExternalAnalog_StatusTypeDef: information if there were any errors
 */
/*
static ExternalAnalog_StatusTypeDef sanity_check_registers(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver, ExternalAnalog_Register0* pReg0, ExternalAnalog_Register1* pReg1, ExternalAnalog_Register2* pReg2, ExternalAnalog_Register3* pReg3) {
    uint8_t data;
    ExternalAnalog_StatusTypeDef status;
    void* registers[] = { pReg0, pReg1, pReg2, pReg3 };
    ExternalAnalog_RegisterAddress addresses[] = { REGISTER0, REGISTER1, REGISTER2, REGISTER3 };

    for (uint8_t i = 0; i < 3; i++) {
        status = command_register_read(pExternalAnalogDriver, addresses[i], &data);
        if (memcmp(&data, registers[i], 1) != 0 || status != EXTERNAL_ANALOG_DRIVER_OK)
            return EXTERNAL_ANALOG_DRIVER_ERROR;
    }

    return EXTERNAL_ANALOG_DRIVER_OK;
}
*/

// Replace HAL_GetTick with xTaskGetTickCount
#define HAL_GetTick() (xTaskGetTickCount() * portTICK_PERIOD_MS)

// Replace HAL_Delay with vTaskDelay
#define HAL_Delay(ms) vTaskDelay(pdMS_TO_TICKS(ms))

/**
 * @brief Calibrate the module by shorting AIN_P and AIN_N inputs,
 * taking CALIBRATION_SAMPLES_AMOUNT samples, averaging the result,
 * saving the offset data and returning the previous register state
 *
 * @param pExternalAnalogDriver: pointer to the driver struct
 *
 * @retval ExternalAnalog_StatusTypeDef: information if there were any errors
 */
static ExternalAnalog_StatusTypeDef calibrate(ExternalAnalog_DriverTypeDef* pExternalAnalogDriver) {
    ExternalAnalog_Register0 register0_previous;
    if (command_register_read(pExternalAnalogDriver, REGISTER0, (uint8_t*)&register0_previous) != EXTERNAL_ANALOG_DRIVER_OK)
        return EXTERNAL_ANALOG_DRIVER_ERROR;

    ExternalAnalog_Register0 register0_calibration = register0_previous;
    register0_calibration.mux = MUX_AIN_PN_SHORTED;
    if (command_register_write(pExternalAnalogDriver, REGISTER0, (uint8_t*)&register0_calibration) != EXTERNAL_ANALOG_DRIVER_OK)
        return EXTERNAL_ANALOG_DRIVER_ERROR;

    int32_t calibration_samples[CALIBRATION_SAMPLES_AMOUNT] = {0};
    uint32_t tick_start = xTaskGetTickCount() * portTICK_PERIOD_MS;

    pExternalAnalogDriver->pCalibrationSamples = calibration_samples;
    pExternalAnalogDriver->isCalibrating = true;

    if (command_start(pExternalAnalogDriver) != EXTERNAL_ANALOG_DRIVER_OK)
        return EXTERNAL_ANALOG_DRIVER_ERROR;

    while (pExternalAnalogDriver->CalibrationSampleIndex < CALIBRATION_SAMPLES_AMOUNT) {
        if ((xTaskGetTickCount() * portTICK_PERIOD_MS) - tick_start > CALIBRATION_MAX_DELAY)
            return EXTERNAL_ANALOG_DRIVER_ERROR;
    }

    int64_t average = 0;
    for (uint16_t i = 0; i < CALIBRATION_SAMPLES_AMOUNT; i++)
        average += calibration_samples[i];
    average /= CALIBRATION_SAMPLES_AMOUNT;

    pExternalAnalogDriver->CalibrationOffset = average;
    pExternalAnalogDriver->isCalibrating = false;
    pExternalAnalogDriver->pCalibrationSamples = NULL;
    pExternalAnalogDriver->CalibrationSampleIndex = 0;

    if (command_powerdown(pExternalAnalogDriver) != EXTERNAL_ANALOG_DRIVER_OK)
        return EXTERNAL_ANALOG_DRIVER_ERROR;

    if (command_register_write(pExternalAnalogDriver, REGISTER0, (uint8_t*)&register0_previous) != EXTERNAL_ANALOG_DRIVER_OK)
        return EXTERNAL_ANALOG_DRIVER_ERROR;

    return EXTERNAL_ANALOG_DRIVER_OK;
}