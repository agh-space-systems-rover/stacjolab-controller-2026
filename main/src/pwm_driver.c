#include "pwm_driver.h"
#include "esp_err.h"
#include <math.h>
#include <stdint.h>


static inline uint32_t pwm_driver_max_duty(const pwm_driver_t *driver) {
    return (1u << driver->duty_resolution) - 1u;
}

esp_err_t pwm_driver_init(pwm_driver_t *driver) {

    if(!driver) return ESP_ERR_INVALID_ARG;

    ledc_timer_config_t timer_conf = {
        .speed_mode = driver->speed_mode,
        .duty_resolution = driver->duty_resolution,
        .timer_num = driver->timer,
        .freq_hz = driver->freq_hz,
        .clk_cfg = LEDC_AUTO_CLK
    };

    esp_err_t err = ledc_timer_config(&timer_conf);
    if(err != ESP_OK) return err;

    ledc_channel_config_t channel_conf = {
        .speed_mode = driver->speed_mode,
        .channel = driver->channel,
        .timer_sel = driver->timer,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = driver->gpio_num,
        .duty = 0,
        .hpoint = 0
    };

    return ledc_channel_config(&channel_conf);
}

esp_err_t pwm_driver_set_duty_raw(const pwm_driver_t *driver, uint32_t duty) {

    if(!driver) return ESP_ERR_INVALID_ARG;

    uint32_t max_duty = pwm_driver_max_duty(driver);
    if(duty > max_duty) duty = max_duty;

    esp_err_t err = ledc_set_duty(driver->speed_mode, driver->channel, duty);
    if(err != ESP_OK) return err;

    return ledc_update_duty(driver->speed_mode, driver->channel);    
}

esp_err_t pwm_driver_set_duty_percent(const pwm_driver_t *driver, float percent) {
    if (!driver) return ESP_ERR_INVALID_ARG;

    if(percent < 0.0f) percent = 0.0f;
    if(percent > 100.0f) percent = 100.0f;

    uint32_t max_duty = pwm_driver_max_duty(driver);
    uint32_t duty = (uint32_t)lroundf((percent/ 100.0f) * (float)max_duty);

    return pwm_driver_set_duty_raw(driver, duty);    
}