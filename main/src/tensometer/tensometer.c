#include <string.h>
#include "tensometer.h"
#include "adc_external_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"

static const char *TAG = "tensometer";

static ExternalAnalog_DriverTypeDef tenso1, tenso2;



// --- Circular Buffer using Sum Tracking (for O(1) SMA) ---
// Formula derived from user request:
// SMA_next = SMA_prev + (p_next - p_oldest)/k
//
// We track the sum directly to avoid integer division errors during updates:
// Sum_next = Sum_prev - p_oldest + p_next
// SMA = Sum_next / k
//
// This allows updating the moving average in O(1) time without looping.

typedef struct {
    int32_t buffer[TENSOMETER_MOVING_AVERAGE_SIZE];
    int head;       // Index of the oldest element
    int count;      // Current number of elements
    int64_t sum;    // Sum of all elements
} CircularBuffer_t;

static CircularBuffer_t cb1 = {0};
static CircularBuffer_t cb2 = {0};

static void cb_add(CircularBuffer_t *cb, int32_t new_val) {
    if (cb->count < TENSOMETER_MOVING_AVERAGE_SIZE) {
        // Buffer not full yet
        cb->buffer[cb->count] = new_val;
        cb->sum += new_val;
        cb->count++;
    } else {
        // Correct implementation of SMA_next = SMA_prev + (p_next - p_oldest)/k 
        // by maintaining Sum_next = Sum_prev - p_oldest + p_next
        int32_t p_oldest = cb->buffer[cb->head];
        int32_t p_next = new_val;
        
        cb->sum = cb->sum - p_oldest + p_next;
        cb->buffer[cb->head] = p_next;
        
        // Move head to the next oldest value for the next iteration
        cb->head = (cb->head + 1) % TENSOMETER_MOVING_AVERAGE_SIZE;
    }
}

static int32_t cb_avg(const CircularBuffer_t *cb) {
    if (cb->count == 0) return 0;
    return (int32_t)(cb->sum / cb->count);
}

static void cb_reset(CircularBuffer_t *cb) {
    cb->head = 0;
    cb->count = 0;
    cb->sum = 0;
    memset(cb->buffer, 0, sizeof(cb->buffer));
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
        .Gain = TENSOMETER_DEFAULT_GAIN,
        .DataRate = TENSOMETER_DEFAULT_DATA_RATE
    };
    
    ExternalAnalog_DriverInitTypeDef init2 = {
        .i2c_port = I2C_NUM_0,
        .PinA0 = EXTERNAL_ANALOG_PIN_DGND,
        .PinA1 = EXTERNAL_ANALOG_PIN_DGND,
        .Mux = MUX_AIN_PN_03, 
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
    ExternalAnalog_Driver_SetMovingAverageSize(TENSOMETER_MOVING_AVERAGE_SIZE);

    // // Perform initial tare
    // for(int i = 0; i < TENSOMETER_MOVING_AVERAGE_SIZE; i++) {
    //     tensometer_read_all();

    //     vTaskDelay(pdMS_TO_TICKS(100));
    // }
    
    // tensometer_tare();
    
    ESP_LOGI(TAG, "Tensometers initialized successfully");
    return ESP_OK;
}

int32_t tensometer_read_all(void) {
    // Perform manual read cycle
    ExternalAnalog_Driver_DataReadyCallback(&tenso1);
    ExternalAnalog_Driver_DataReadyCallback(&tenso2);
    
    cb_add(&cb1, tenso1.Voltage);
    cb_add(&cb2, tenso2.Voltage);

    ESP_LOGD(TAG, "Read T1: %ld uV (raw: %ld), T2: %ld uV (raw: %ld)", 
             tenso1.Voltage, tenso1.ADCValueFiltered, 
             tenso2.Voltage, tenso2.ADCValueFiltered);
    
    return tensometer_get_voltage_sum();
}

int32_t tensometer_get_voltage_1(void) {
    if (!tenso1.IsDataValid) return 0;
    return cb_avg(&cb1);
}

int32_t tensometer_get_voltage_2(void) {
    if (!tenso2.IsDataValid) return 0;
    return cb_avg(&cb2);
}

int32_t tensometer_get_voltage_sum(void) {
    return tensometer_get_voltage_1() - tensometer_get_voltage_2(); // TODO: Verify calculation
}

void tensometer_tare(void) {
    // 1. Reset circular buffers to start fresh
    cb_reset(&cb1);
    cb_reset(&cb2);

    // 2. Read samples to fill the buffer and get a stable average
    for(int i = 0; i < TENSOMETER_MOVING_AVERAGE_SIZE; i++) {
        tensometer_read_all();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    // 3. Calculate the average voltage (relative to current offset)
    int32_t avg_v1 = cb_avg(&cb1);
    int32_t avg_v2 = cb_avg(&cb2);

    // 4. Update the offsets in the driver
    // NewOffset = OldOffset + AverageVoltage
    tenso1.VoltageOffset += avg_v1;
    tenso2.VoltageOffset += avg_v2;

    // 5. Reset buffers again so subsequent readings start from 0
    cb_reset(&cb1);
    cb_reset(&cb2);
    
    ESP_LOGI(TAG, "Tared both tensometers: offsets T1=%ld, T2=%ld", tenso1.VoltageOffset, tenso2.VoltageOffset);
}

void tensometer_task(void* arg) {
    ESP_LOGI(TAG, "Tensometer task started");
    while(1) {
        tensometer_read_all();
        
        static TickType_t last_log = 0;
        TickType_t now = xTaskGetTickCount();
        if ((now - last_log) > pdMS_TO_TICKS(TENSOMETER_LOG_TIME)) {
            int32_t v1 = tensometer_get_voltage_1();
            int32_t v2 = tensometer_get_voltage_2();
            ESP_LOGI(TAG, "Avg V1: %ld uV, Avg V2: %ld uV, Diff Same: %ld", 
                     v1, v2, v1 - v2);
            last_log = now;
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}
