#include "pwm_driver.h"
#include "power_switch.h"
#include "pin_def.h"
#include "stacjolab.h"

static const char* TAG = "power_switch";

esp_err_t power_switch_init(power_switch_t *power_switch, uint8_t id, ledc_timer_t timer, ledc_channel_t channel, gpio_num_t gpio_num, uint16_t freq_hz, ledc_timer_bit_t duty_resolution) {
    power_switch->id = id;
    power_switch->enabled = 0;

    power_switch->pwm_driver.speed_mode = LEDC_LOW_SPEED_MODE;
    power_switch->pwm_driver.timer = timer;
    power_switch->pwm_driver.channel = channel;
    power_switch->pwm_driver.gpio_num = gpio_num;
    power_switch->pwm_driver.freq_hz = freq_hz;
    power_switch->pwm_driver.duty_resolution = duty_resolution;

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