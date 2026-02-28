#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "adc_external_driver.h" // For GAIN/DATA_RATE definitions

// =================================================================================================
// Configuration Macros
// =================================================================================================

/**
 * @brief Default Gain Setting for the ADS122C04
 * Options: GAIN1, GAIN2, GAIN4, GAIN8, GAIN16, GAIN32, GAIN64, GAIN128
 */
#define TENSOMETER_DEFAULT_GAIN GAIN64

/**
 * @brief Default Data Rate Setting
 * Options: DATA_RATE_20_SPS, DATA_RATE_45_SPS, DATA_RATE_90_SPS, DATA_RATE_175_SPS, etc.
 * Note: Higher data rates reduce conversion time but increase noise.
 * Conversion time approx = 1 / SPS.
 */
#define TENSOMETER_DEFAULT_DATA_RATE DATA_RATE_175_SPS

/**
 * @brief Number of samples for the moving average filter.
 * Larger size = smoother data but slower response.
 */
#define TENSOMETER_MOVING_AVERAGE_SIZE 10

/**
 * @brief Number of samples to discard/average during taring process.
 * Ensures the filter is stable before setting the offset.
 */
#define TENSOMETER_TARE_SAMPLES 20


// =================================================================================================
// Function Declarations
// =================================================================================================

/**
 * @brief Initialize the tensometer driver (I2C and ADS122C04).
 * 
 * This function initializes the I2C master bus, configures the two ADC channels (Strain Gauge 1 & 2),
 * sets up the moving average filter, and performs an initial tare.
 * 
 * @param sda_pin GPIO number for I2C SDA
 * @param scl_pin GPIO number for I2C SCL
 * @return esp_err_t ESP_OK on success, ESP_FAIL or other error code on failure.
 */
esp_err_t tensometer_init(int sda_pin, int scl_pin);

/**
 * @brief Perform a complete read cycle for both tensometers.
 * 
 * This function triggers a conversion on both ADC channels, waits for the result,
 * and updates the internal voltage values. It is a blocking function.
 * Execution time depends on the data rate (approx 2 * (1/DataRate) + overhead).
 * For 20 SPS, this is approx 100-110ms.
 * 
 * @return int32_t Sum of voltages from both tensometers (in microvolts).
 */
int32_t tensometer_read_all(void);

/**
 * @brief Get the last read voltage for Tensometer 1.
 * 
 * @return int32_t Voltage in microvolts (uV). Returns 0 if data invalid.
 */
int32_t tensometer_get_voltage_1(void);

/**
 * @brief Get the last read voltage for Tensometer 2.
 * 
 * @return int32_t Voltage in microvolts (uV). Returns 0 if data invalid.
 */
int32_t tensometer_get_voltage_2(void);


/**
 * @brief Get the sum of voltages from both tensometers.
 * 
 * Useful for dual-beam setups where total weight is distributed across two gauges.
 * 
 * @return int32_t Sum of voltages in microvolts (uV).
 */
int32_t tensometer_get_voltage_sum(void);

/**
 * @brief Tare the tensometers.
 * 
 * Sets the current reading as the zero offset.
 * This function blocks while reading TENSOMETER_TARE_SAMPLES to ensure a stable baseline.
 */
void tensometer_tare(void);

/**
 * @brief FreeRTOS task for debugging tensometer readings.
 * 
 * This task continuously reads from the tensometers and logs the values
 * to the console every second. It is intended for testing and verification purposes.
 * 
 * @param arg Unused parameter.
 */
void tensometer_task(void* arg);