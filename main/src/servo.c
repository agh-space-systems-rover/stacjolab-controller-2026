#include "servo.h"

#include "esp_log.h"
#include "esp_err.h"

esp_err_t servo_init(servo_t* servo, gpio_num_t pin, ledc_timer_t timer, ledc_channel_t channel, uint16_t freq_hz, ledc_timer_bit_t duty_resolution) {
    if(servo == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    servo->pwm.speed_mode = LEDC_LOW_SPEED_MODE;
    servo->pwm.timer = timer;
    servo->pwm.channel = channel;
    servo->pwm.gpio_num = pin;
    servo->pwm.freq_hz = freq_hz;
    servo->pwm.duty_resolution = duty_resolution;

    ESP_ERROR_CHECK(pwm_driver_init(&servo->pwm));

    return ESP_OK;
}

static uint32_t pulse_width_to_duty(servo_t* servo, uint32_t pulse_width_us) {
    // Convert pulse width in microseconds to duty cycle based on the timer's resolution and frequency
    uint32_t max_duty = (1 << servo->pwm.duty_resolution) - 1; // Max duty based on resolution
    uint32_t period_us = 1000000 / servo->pwm.freq_hz; // Period in microseconds
    return (pulse_width_us * max_duty) / period_us;
}

esp_err_t servo_set_pulse_width(servo_t* servo, uint32_t pulse_width_us) {
    if(servo == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t duty = pulse_width_to_duty(servo, pulse_width_us);
    return pwm_driver_set_duty_raw(&servo->pwm, duty);
}


esp_err_t servo_disable(servo_t* servo) {
    if(servo == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return servo_set_pulse_width(servo, 0);
}

esp_err_t servo_set_percent(servo_t* servo, uint8_t angle) {
    if(servo == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if(angle > SERVO_0_MAX_ANGLE) {
        return ESP_ERR_INVALID_ARG;
    }

    // Map angle to pulse width
    uint32_t pulse_width_us = SERVO_0_MIN_PULSE_WIDTH_US + ((SERVO_0_MAX_PULSE_WIDTH_US - SERVO_0_MIN_PULSE_WIDTH_US) * angle) / SERVO_0_MAX_ANGLE;
    return servo_set_pulse_width(servo, pulse_width_us);
}