/*
 * ExternalAnalog_Registers.h
 *
 *  Created on: Jul 30, 2024
 *      Author: Jakub Halfar
 */

 #ifndef ADC_EXTERNAL_REGISTERS_H_
 #define ADC_EXTERNAL_REGISTERS_H_
 
 #include <stdint.h>
 /**
  * @brief Module commands
  */
 typedef enum {
     COMMAND_RESET       = 0x06, /* Reset the device to the default state */
     COMMAND_START_SYNC  = 0x08, /* Start a single conversion or start the continuous conversion (depending on the chosen mode) */
     COMMAND_POWERDOWN   = 0x02, /* Place the device into power-down mode */
     COMMAND_RDATA       = 0x10, /* Read the most recent conversion result */
     COMMAND_RREG_BASE   = 0x20, /* Read a register */
     COMMAND_WREG_BASE   = 0x40  /* Write to a register */
 } ExternalAnalog_Command;
 
 /**
  * @brief Available register addresses
  */
 typedef enum {
     REGISTER0           = 0x00, /* Configuration register 0 address */
     REGISTER1           = 0x01, /* Configuration register 1 address */
     REGISTER2           = 0x02, /* Configuration register 2 address */
     REGISTER3           = 0x03  /* Configuration register 3 address */
 } ExternalAnalog_RegisterAddress;
 
 /**
  * @brief Configuration register 0
  */
 typedef struct __packed {
     uint8_t pga_bypass  : 1;    /* Disables and bypasses the internal low-noise PGA, RW, 1 bit */
     uint8_t gain        : 3;    /* Gain configuration, RW, 3 bits */
     uint8_t mux         : 4;    /* Input multiplexer configuration, RW, 4 bits */
 } ExternalAnalog_Register0;
 
 /**
  * @brief Configuration register 1
  */
 typedef struct __packed {
     uint8_t ts          : 1;    /* Temperature sensor mode, RW, 1 bit */
     uint8_t vref        : 2;    /* Voltage reference selection, RW, 2 bits */
     uint8_t cm          : 1;    /* Conversion mode, RW, 1 bit */
     uint8_t mode        : 1;    /* Operating mode, RW, 1 bit */
     uint8_t dr          : 3;    /* Data rate, RW, 3 bits */
 } ExternalAnalog_Register1;
 
 /**
  * @brief Configuration register 2
  */
 typedef struct __packed {
     uint8_t idac        : 3;    /* IDAC current setting, RW, 3 bits */
     uint8_t bcs         : 1;    /* Burn-out current sources, RW, 1 bit */
     uint8_t crc         : 2;    /* Data integrity check enable, RW, 2 bits */
     uint8_t dcnt        : 1;    /* Data counter enable, RW, 1 bit */
     uint8_t drdy        : 1;    /* Conversion result ready flag, R, 1 bit */
 } ExternalAnalog_Register2;
 
 /**
  * @brief Configuration register 3
  */
 typedef struct __packed {
     uint8_t _reserved   : 2;    /* Reserved, R, 2 bits, ALWAYS WRITE 0 */
     uint8_t i2mux       : 3;    /* IDAC2 routing configuration, RW, 3 bits */
     uint8_t i1mux       : 3;    /* IDAC1 routing configuration, RW, 3 bits */
 } ExternalAnalog_Register3;
 
 /**
  * @brief Input multiplexer configuration (see register 0)
  *
  * These bits configure the input multiplexer.
  * For settings where AINN = AVSS, the PGA must be disabled (PGA_BYPASS = 1) and only gains 1, 2, and 4 can be used.
  */
 typedef enum {
     MUX_AIN_PN_01       = 0x00, /* AINP = AIN0, AINN = AIN1 (default) */
     MUX_AIN_PN_02       = 0x01, /* AINP = AIN0, AINN = AIN2 */
     MUX_AIN_PN_03       = 0x02, /* AINP = AIN0, AINN = AIN3 */
     MUX_AIN_PN_10       = 0x03, /* AINP = AIN1, AINN = AIN0 */
     MUX_AIN_PN_12       = 0x04, /* AINP = AIN1, AINN = AIN2 */
     MUX_AIN_PN_13       = 0x05, /* AINP = AIN1, AINN = AIN3 */
     MUX_AIN_PN_23       = 0x06, /* AINP = AIN2, AINN = AIN3 */
     MUX_AIN_PN_32       = 0x07, /* AINP = AIN3, AINN = AIN2 */
     MUX_AIN_PN_0_VSS    = 0x08, /* AINP = AIN0, AINN = AVSS */
     MUX_AIN_PN_1_VSS    = 0x09, /* AINP = AIN1, AINN = AVSS */
     MUX_AIN_PN_2_VSS    = 0x0A, /* AINP = AIN2, AINN = AVSS */
     MUX_AIN_PN_3_VSS    = 0x0B, /* AINP = AIN3, AINN = AVSS */
     MUX_VREFP_VREFN_4   = 0x0C, /* (VREFP - VREFN) / 4 monitor (PGA bypassed) */
     MUX_VDD_VSS_4       = 0x0D, /* (AVDD - AVSS) / 4 monitor (PGA bypassed) */
     MUX_AIN_PN_SHORTED  = 0x0E, /* AINP and AINN shorted to (AVDD + AVSS) / 2 */
 } ExternalAnalog_Mux;
 
 /**
  * @brief Gain configuration (see register 0)
  *
  * These bits configure the device gain.
  * Gains 1, 2, and 4 can be used without the PGA. In this case, gain is obtained by a switched-capacitor structure.
  */
 typedef enum {
     GAIN1               = 0x00, /* Gain = 1 (default) */
     GAIN2               = 0x01, /* Gain = 2 */
     GAIN4               = 0x02, /* Gain = 4 */
     GAIN8               = 0x03, /* Gain = 8 */
     GAIN16              = 0x04, /* Gain = 16 */
     GAIN32              = 0x05, /* Gain = 32 */
     GAIN64              = 0x06, /* Gain = 64 */
     GAIN128             = 0x07  /* Gain = 128 */
 } ExternalAnalog_Gain;
 
 /**
  * @brief Operating mode (see register 1)
  */
 typedef enum {
     NORMAL_MODE         = 0x00, /* Normal mode (default) */
     TURBO_MODE          = 0x01  /* Turbo mode */
 } ExternalAnalog_OperatingMode;
 
 /**
  * @brief Conversion mode (see register 1)
  *
  * This bit sets the conversion mode for the device.
  */
 typedef enum {
     SINGLE_SHOT_MODE    = 0x00, /* Single-shot conversion mode (default) */
     CONTINUOUS_MODE     = 0x01  /* Continuous conversion mode */
 } ExternalAnalog_ConversionMode;
 
 /**
  * @brief Voltage reference selection (see register 1)
  *
  * These bits select the voltage reference source that is used for the conversion.
  */
 typedef enum {
     VREF_INTERNAL       = 0x00, /* Internal 2.048-V reference selected (default) */
     VREF_EXTERNAL       = 0x01, /* External reference selected using the REFP and REFN inputs */
     VREF_ANALOG_SUPPLY1 = 0x02, /* Analog supply (AVDD - AVSS) used as reference */
     VREF_ANALOG_SUPPLY2 = 0x03, /* Analog supply (AVDD - AVSS) used as reference */
 } ExternalAnalog_VoltageReferenceSelection;
 
 /**
  * @brief Temperature sensor mode (see register 1)
  *
  * This bit enables the internal temperature sensor and puts the device in temperature sensor mode.
  * The settings of configuration register 0 have no effect and the device uses the
  * internal reference for measurement when temperature sensor mode is enabled.
  */
 typedef enum {
     TEMP_SENSOR_DISABLE = 0x00, /* Temperature sensor mode disabled (default) */
     TEMP_SENSOR_ENABLE  = 0x01  /* Temperature sensor mode enabled */
 } ExternalAnalog_TemperatureSensorMode;
 
 /**
  * @brief Data rate (see register 1)
  */
 typedef enum {
     DATA_RATE_20_SPS    = 0x00,
     DATA_RATE_45_SPS    = 0x01,
     DATA_RATE_90_SPS    = 0x02,
     DATA_RATE_175_SPS   = 0x03, /* 175 samples per second */
     DATA_RATE_330_SPS   = 0x04,
     DATA_RATE_600_SPS   = 0x05,
     DATA_RATE_1000_SPS  = 0x06
 } ExternalAnalog_DataRate;
 
 /**
  * @brief Conversion result ready flag (see register 2)
  *
  * This bit flags if a new conversion result is ready. This bit is reset when conversion data are read.
  */
 typedef enum {
     DATA_NOT_READY      = 0x00, /* No new conversion result available (default) */
     DATA_READY          = 0x01  /* New conversion result ready */
 } ExternalAnalog_DataReadyFlag;
 
 /**
  * @brief Data counter enable (see register 2)
  *
  * The bit enables the conversion data counter.
  */
 typedef enum {
     COUNTER_DISABLE     = 0x00, /* Conversion counter disabled (default) */
     COUNTER_ENABLE      = 0x01  /* Conversion counter enabled */
 } ExternalAnalog_DataCounter;
 
 /**
  * @brief Data integrity check enable (see register 2)
  *
  * These bits enable and select the data integrity checks.
  */
 typedef enum {
     INTEGRITY_DISABLE   = 0x00, /* Disabled (default) */
     INTEGRITY_INVERT    = 0x01, /* Inverted data output enabled */
     INTEGRITY_CRC16     = 0x02  /* CRC16 enabled */
 } ExternalAnalog_DataIntegrityCheck;
 
 /**
  * @brief Burn-out current sources (see register 2)
  *
  * This bit controls the 10-μA, burn-out current sources. The burn-out current
  * sources can be used to detect sensor faults such as wire breaks and shorted
  * sensors.
  */
 typedef enum {
     BCS_DISABLE         = 0x00, /* Current sources off (default) */
     BCS_ENABLE          = 0x01, /* Current sources on */
 } ExternalAnalog_BurnOutCurrentSources;
 
 /**
  * @brief IDAC current setting (see register 2)
  *
  * These bits set the current for both IDAC1 and IDAC2 excitation current sources.
  */
 typedef enum {
     IDAC_OFF            = 0x00, /* Off (default) */
     IDAC_10             = 0x01, /* 10 uA */
     IDAC_50             = 0x02, /* 50 uA */
     IDAC_100            = 0x03, /* 100 uA */
     IDAC_250            = 0x04, /* 250 uA */
     IDAC_500            = 0x05, /* 500 uA */
     IDAC_1000           = 0x06, /* 1000 uA */
     IDAC_1500           = 0x07, /* 1500 uA */
 } ExternalAnalog_IDACCurrentSetting;
 
 /**
  * @brief IDAC1 routing configuration (see register 3)
  *
  * These bits select the channel that IDAC1 is routed to.
  */
 typedef enum {
     IDAC1_OFF           = 0x00, /* IDAC1 disabled (default) */
     IDAC1_AIN0          = 0x01, /* IDAC1 connected to AIN0 */
     IDAC1_AIN1          = 0x02, /* IDAC1 connected to AIN1 */
     IDAC1_AIN2          = 0x03, /* IDAC1 connected to AIN2 */
     IDAC1_AIN3          = 0x04, /* IDAC1 connected to AIN3 */
     IDAC1_REFP          = 0x05, /* IDAC1 connected to REFP */
     IDAC1_REFN          = 0x06, /* IDAC1 connected to REFN */
 } ExternalAnalog_IDAC1Routing;
 
 /**
  * @brief IDAC2 routing configuration (see register 3)
  *
  * These bits select the channel that IDAC2 is routed to.
  */
 typedef enum {
     IDAC2_OFF           = 0x00, /* IDAC2 disabled (default) */
     IDAC2_AIN0          = 0x01, /* IDAC2 connected to AIN0 */
     IDAC2_AIN1          = 0x02, /* IDAC2 connected to AIN1 */
     IDAC2_AIN2          = 0x03, /* IDAC2 connected to AIN2 */
     IDAC2_AIN3          = 0x04, /* IDAC2 connected to AIN3 */
     IDAC2_REFP          = 0x05, /* IDAC2 connected to REFP */
     IDAC2_REFN          = 0x06, /* IDAC2 connected to REFN */
 } ExternalAnalog_IDAC2Routing;
 
 /**
  * @brief Base I2C device address
  */
 #define I2C_SLAVE_BASE_ADDRESS 0x40
 
 /**
  * @brief Number of bytes of received data (24 bits)
  */
 #define CONVERTED_DATA_SIZE 3
 
 #endif /* ADC_EXTERNAL_REGISTERS_H_ */