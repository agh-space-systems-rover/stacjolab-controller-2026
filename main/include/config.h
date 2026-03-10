#pragma once

/*
    WIFI configuration
*/

#define WIFI_CHANNEL 1

/*
    Temperature control configuration
*/

#define MAX_HEATER_TEMP 150
#define STACJOLAB_HIGH_TEMP_THRESHOLD 100
#define STACJOLAB_LOW_TEMP_THRESHOLD 97
#define STACJOLAB_HEATER_DUTY_CYCLE 85
#define STACJOLAB_LID_HEATER_DUTY_CYCLE 50

/*
    Servo configuration
*/
#define SERVO_0_MAX_ANGLE 100
#define SERVO_0_MAX_PULSE_WIDTH_US 2100
#define SERVO_0_MIN_PULSE_WIDTH_US 900

/*
    LED indicator configuration
*/

#define LED_STRIP_SIZE 60

#define STACJOLAB_RED_ON_BOOT 255
#define STACJOLAB_GREEN_ON_BOOT 80
#define STACJOLAB_BLUE_ON_BOOT 0