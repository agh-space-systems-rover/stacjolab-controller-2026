#include "tensometer.h"
#include "adc_external_driver.h"
// #include "pin_def.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

static const char *TAG = "tensometer";

static ExternalAnalog_DriverTypeDef tenso1, tenso2;
static int sda_pin;
static int scl_pin;

// Forward declarations
int32_t tensometer_get_voltage_1(void);
int32_t tensometer_get_voltage_2(void);
int32_t tensometer_get_voltage_sum(void);

// Callbacks (optional, currently just logging)
static void tenso1_callback(int32_t value) {
    // ESP_LOGD(TAG, "Callback T1: %ld uV", value);
}

static void tenso2_callback(int32_t value) {
    // ESP_LOGD(TAG, "Callback T2: %ld uV", value);
}

static void tensometer_task(void *arg) {
    // Initialize I2C for ADC
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);

    // Initialize Tensometers
    // Beam 1: AIN1 (Positive) / AIN2 (Negative) -> MUX_AIN_PN_12
    ExternalAnalog_DriverInitTypeDef init1 = {
        .i2c_port = I2C_NUM_0,
        .PinA0 = EXTERNAL_ANALOG_PIN_DGND,
        .PinA1 = EXTERNAL_ANALOG_PIN_DGND,
        .Mux = MUX_AIN_PN_12, 
        .DataReadyCallback = tenso1_callback,
        .Gain = GAIN64,
        .DataRate = DATA_RATE_175_SPS
    };
    
    // Beam 2: AIN0 (Positive) / AIN3 (Negative) -> MUX_AIN_PN_03
    ExternalAnalog_DriverInitTypeDef init2 = {
        .i2c_port = I2C_NUM_0,
        .PinA0 = EXTERNAL_ANALOG_PIN_DGND,
        .PinA1 = EXTERNAL_ANALOG_PIN_DGND,
        .Mux = MUX_AIN_PN_03, 
        .DataReadyCallback = tenso2_callback,
        .Gain = GAIN64,
        .DataRate = DATA_RATE_175_SPS
    };

    ExternalAnalog_Driver_Init(&tenso1, &init1);
    ExternalAnalog_Driver_Init(&tenso2, &init2);

    // Set faster moving average for testing
    ExternalAnalog_Driver_SetMovingAverageSize(10); 

    while(1) {
        if(tenso1.isModuleInitialized) {
            ExternalAnalog_Driver_DataReadyCallback(&tenso1);
        }
        if(tenso2.isModuleInitialized) {
            ExternalAnalog_Driver_DataReadyCallback(&tenso2);
        }

        // Always print voltage for testing
        static TickType_t last_print = 0;
        if ((xTaskGetTickCount() - last_print) >= pdMS_TO_TICKS(500)) {
             ESP_LOGI(TAG, "Voltage 1: %ld uV, Voltage 2: %ld uV, Sum: %ld uV", 
                      tenso1.Voltage, tenso2.Voltage, tenso1.Voltage + tenso2.Voltage);
             last_print = xTaskGetTickCount();
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Sample every 10ms
    }
}

void tensometer_init(int sda, int scl) {
    sda_pin = sda;
    scl_pin = scl;
    xTaskCreate(tensometer_task, "tensometer_task", 4096, NULL, 5, NULL);
}

int32_t tensometer_get_voltage_1(void) {
    if (!tenso1.IsDataValid) return 0;
    return tenso1.Voltage;
}

int32_t tensometer_get_voltage_2(void) {
    if (!tenso2.IsDataValid) return 0;
    return tenso2.Voltage;
}

int32_t tensometer_get_voltage_sum(void) {
    return tensometer_get_voltage_1() + tensometer_get_voltage_2();
}

void tensometer_tare(void) {
    ExternalAnalog_Driver_Tare(&tenso1);
    ExternalAnalog_Driver_Tare(&tenso2);
    ESP_LOGI(TAG, "Tared both tensometers");
}
