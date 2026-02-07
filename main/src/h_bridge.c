#include "stacjolab.h"
#include "h_bridge.h"
#include "pin_def.h"

static ledc_channel_t h_bridge_pwm1_channels[] = {LEDC_CHANNEL_0, LEDC_CHANNEL_1};
static ledc_channel_t h_bridge_pwm2_channels[] = {LEDC_CHANNEL_2, LEDC_CHANNEL_3};
static gpio_num_t h_bridge_pwm1_pins[] = {H_BRIDGE_0_PWM1_PIN, H_BRIDGE_1_PWM1_PIN};
static gpio_num_t h_bridge_pwm2_pins[] = {H_BRIDGE_0_PWM2_PIN, H_BRIDGE_1_PWM2_PIN};

esp_err_t h_bridge_init(h_bridge_t* h_bridge, uint8_t h_bridge_id) {
    if(h_bridge == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    h_bridge->h_bridge_id = h_bridge_id;

    h_bridge->pwm_driver1.speed_mode = LEDC_LOW_SPEED_MODE;
    h_bridge->pwm_driver1.timer = LEDC_TIMER_0;
    h_bridge->pwm_driver1.channel = h_bridge_pwm1_channels[h_bridge_id];
    h_bridge->pwm_driver1.gpio_num = h_bridge_pwm1_pins[h_bridge_id];
    h_bridge->pwm_driver1.freq_hz = LEDC_TIMER_0_FREQ;
    h_bridge->pwm_driver1.duty_resolution = LEDC_TIMER_10_BIT;

    h_bridge->pwm_driver2.speed_mode = LEDC_LOW_SPEED_MODE;
    h_bridge->pwm_driver2.timer = LEDC_TIMER_0;
    h_bridge->pwm_driver2.channel = h_bridge_pwm2_channels[h_bridge_id];
    h_bridge->pwm_driver2.gpio_num = h_bridge_pwm2_pins[h_bridge_id];
    h_bridge->pwm_driver2.freq_hz = LEDC_TIMER_0_FREQ;
    h_bridge->pwm_driver2.duty_resolution = LEDC_TIMER_10_BIT;

    pwm_driver_init(&h_bridge->pwm_driver1);
    pwm_driver_init(&h_bridge->pwm_driver2);

    return ESP_OK;
}

h_bridge_t* get_h_bridge_by_id(uint8_t h_bridge_id) {
    switch(h_bridge_id) {
        case H_BRIDGE_0_ID:
            return &stacjolab_controller.h_bridge_ch_0;
        case H_BRIDGE_1_ID:
            return &stacjolab_controller.h_bridge_ch_1;
        default:
            return NULL;
    }
}

esp_err_t h_bridge_set_speed(h_bridge_t* h_bridge, int8_t speed) {
    if(h_bridge == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Clamp speed to -100 to 100
    if(speed > 100) speed = 100;
    if(speed < -100) speed = -100;

    esp_err_t err;
    if(speed >= 0) {
        err = pwm_driver_set_duty_percent(&h_bridge->pwm_driver1, (float)speed);
        if(err != ESP_OK) return err;
        err = pwm_driver_set_duty_percent(&h_bridge->pwm_driver2, 0.0f);
        if(err != ESP_OK) return err;
    } else {
        err = pwm_driver_set_duty_percent(&h_bridge->pwm_driver1, 0.0f);
        if(err != ESP_OK) return err;
        err = pwm_driver_set_duty_percent(&h_bridge->pwm_driver2, (float)(-speed));
        if(err != ESP_OK) return err;
    }

    return err;
}