#include "cc_driver.h"
#include "stacjolab.h"
#include "pin_def.h"

esp_err_t cc_driver_init(cc_driver_t *cc_driver) {
    
    cc_driver->pwm_driver.speed_mode = LEDC_LOW_SPEED_MODE;
    cc_driver->pwm_driver.timer = LEDC_TIMER_1;
    cc_driver->pwm_driver.channel = LEDC_CHANNEL_4;
    cc_driver->pwm_driver.gpio_num = CC_PWM_PIN;
    cc_driver->pwm_driver.freq_hz = LEDC_TIMER_0_FREQ;
    cc_driver->pwm_driver.duty_resolution = LEDC_TIMER_10_BIT;

    return pwm_driver_init(&cc_driver->pwm_driver);
}

esp_err_t cc_driver_duty(cc_driver_t *cc_driver, float duty) {
    return pwm_driver_set_duty_percent(&cc_driver->pwm_driver, duty);
}