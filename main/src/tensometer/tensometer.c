#include "tensometer.h"
#include "adc_external_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"

static const char *TAG = "tensometer";

static ExternalAnalog_DriverTypeDef tenso1, tenso2;

// Callbacks (internal, optional logging)
static void tenso1_callback(int32_t value) {
    // ESP_LOGD(TAG, "T1 Raw: %ld", value);
}

static void tenso2_callback(int32_t value) {
    // ESP_LOGD(TAG, "T2 Raw: %ld", value);
}

esp_err_t tensometer_init(int sda_pin, int scl_pin) {
    // 1. Initialize I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    esp_err_t err = i2c_param_config(I2C_NUM_0, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed");
        return err;
    }
    
    err = i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed");
        return err;
    }

    // 2. Initialize Drivers
    ExternalAnalog_DriverInitTypeDef init1 = {
        .i2c_port = I2C_NUM_0,
        .PinA0 = EXTERNAL_ANALOG_PIN_DGND,
        .PinA1 = EXTERNAL_ANALOG_PIN_DGND,
        .Mux = MUX_AIN_PN_12, 
        .DataReadyCallback = tenso1_callback,
        .Gain = TENSOMETER_DEFAULT_GAIN,
        .DataRate = TENSOMETER_DEFAULT_DATA_RATE
    };
    
    ExternalAnalog_DriverInitTypeDef init2 = {
        .i2c_port = I2C_NUM_0,
        .PinA0 = EXTERNAL_ANALOG_PIN_DGND,
        .PinA1 = EXTERNAL_ANALOG_PIN_DGND,
        .Mux = MUX_AIN_PN_03, 
        .DataReadyCallback = tenso2_callback,
        .Gain = TENSOMETER_DEFAULT_GAIN,
        .DataRate = TENSOMETER_DEFAULT_DATA_RATE
    };

    if (ExternalAnalog_Driver_Init(&tenso1, &init1) != EXTERNAL_ANALOG_DRIVER_OK) {
        ESP_LOGE(TAG, "Tensometer 1 init failed");
        return ESP_FAIL;
    }
    
    if (ExternalAnalog_Driver_Init(&tenso2, &init2) != EXTERNAL_ANALOG_DRIVER_OK) {
        ESP_LOGE(TAG, "Tensometer 2 init failed");
        return ESP_FAIL;
    }

    // Set moving average size
    ExternalAnalog_Driver_SetMovingAverageSize(1); // TENSOMETER_MOVING_AVERAGE_SIZE

    // Perform initial tare
    for(int i = 0; i < TENSOMETER_TARE_SAMPLES; i++) {
        tensometer_read_all();

        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    tensometer_tare();
    
    ESP_LOGI(TAG, "Tensometers initialized successfully");
    return ESP_OK;
}

int32_t tensometer_read_all(void) {
    // Perform manual read cycle
    ExternalAnalog_Driver_DataReadyCallback(&tenso1);
    ExternalAnalog_Driver_DataReadyCallback(&tenso2);
    
    ESP_LOGI(TAG, "Read T1: %ld uV (raw: %ld), T2: %ld uV (raw: %ld)", 
             tenso1.Voltage, tenso1.ADCValueFiltered, 
             tenso2.Voltage, tenso2.ADCValueFiltered);
    return tensometer_get_voltage_sum();
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
    return tensometer_get_voltage_1() - tensometer_get_voltage_2(); // TODO: Verify the correct calculation
}

void tensometer_tare(void) {
    // Read a few samples to settle filter
    for(int i = 0; i < TENSOMETER_TARE_SAMPLES; i++) {
        tensometer_read_all();
        // tensometer_read_all already blocks sufficiently (approx 110ms)
    }
    
    ExternalAnalog_Driver_Tare(&tenso1);
    vTaskDelay(pdMS_TO_TICKS(100)); // Short delay to ensure second tare is not affected by first
    ExternalAnalog_Driver_Tare(&tenso2);
    ESP_LOGI(TAG, "Tared both tensometers: offsets T1=%ld, T2=%ld", tenso1.VoltageOffset, tenso2.VoltageOffset);
}

void tensometer_task(void* arg) {
    // Loop for debugging
    while(1) {
        tensometer_read_all();
        
        static TickType_t last_log = 0;
        if ((xTaskGetTickCount() - last_log) > pdMS_TO_TICKS(1000)) {
            ESP_LOGI(TAG, "V1: %ld uV, V2: %ld uV, Sum: %ld uV", 
                     tenso1.Voltage, tenso2.Voltage, tenso1.Voltage + tensometer_get_voltage_2());
            last_log = xTaskGetTickCount();
        }
        
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}
