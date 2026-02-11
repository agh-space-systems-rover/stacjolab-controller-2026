#include "pwm_driver.h"
#include "power_switch.h"
#include "pin_def.h"
#include "stacjolab.h"

static const char* TAG = "power_switch";

static ledc_channel_t power_switch_pwm_channels[] = {LEDC_CHANNEL_5, LEDC_CHANNEL_6, LEDC_CHANNEL_7};
static gpio_num_t power_switch_pwm_pins[] = {PS_PWM_1_PIN, PS_PWM_2_PIN, PS_PWM_3_PIN};

esp_err_t power_switch_init(power_switch_t *power_switch, uint8_t id) {
    power_switch->id = id;
    power_switch->enabled = 0;

    power_switch->pwm_driver.speed_mode = LEDC_LOW_SPEED_MODE;
    power_switch->pwm_driver.timer = LEDC_TIMER_0;
    power_switch->pwm_driver.channel = power_switch_pwm_channels[id];
    power_switch->pwm_driver.gpio_num = power_switch_pwm_pins[id];
    power_switch->pwm_driver.freq_hz = LEDC_TIMER_0_FREQ;
    power_switch->pwm_driver.duty_resolution = LEDC_TIMER_10_BIT;

    return pwm_driver_init(&power_switch->pwm_driver);
}

power_switch_t* get_power_switch_by_id(uint8_t id) {
    switch (id){
        case POWER_SWITCH_0_ID:
            return &stacjolab_controller.power_switch_ch_0;
        case POWER_SWITCH_1_ID:
            return &stacjolab_controller.power_switch_ch_1;
        case POWER_SWITCH_2_ID:
            return &stacjolab_controller.power_switch_ch_2;
        default:
            return NULL;
    }
}

esp_err_t power_switch_set_duty(power_switch_t *power_switch, float duty) {
    power_switch->duty = duty;
    return pwm_driver_set_duty_percent(&power_switch->pwm_driver, duty);
}

esp_err_t power_switch_enable(power_switch_t *power_switch, uint8_t enabled) {
    power_switch->enabled = enabled;
    if (enabled) {
        return pwm_driver_set_duty_percent(&power_switch->pwm_driver, power_switch->duty);
    } else {
        return pwm_driver_set_duty_raw(&power_switch->pwm_driver, 0);
    }
}