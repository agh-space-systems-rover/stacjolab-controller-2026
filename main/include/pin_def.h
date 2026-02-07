#pragma once

#include "driver/gpio.h"

// Pin definitions

// Debug LED
#define LED1_PIN                GPIO_NUM_21
#define LED2_PIN                45

// Auxiliary header pins
#define AUX_1_PIN               GPIO_NUM_1
#define AUX_2_PIN               GPIO_NUM_3
#define AUX_3_PIN               GPIO_NUM_11
#define AUX_4_PIN               GPIO_NUM_2
#define AUX_5_PIN               GPIO_NUM_14
#define AUX_6_PIN               GPIO_NUM_13

// WS2812 LED strip
#define WS2812_PIN              46

// Tenso amp
#define TENSO_SCL_PIN           GPIO_NUM_9
#define TENSO_SDA_PIN           GPIO_NUM_10
#define TENSO_nDRDY_PIN         GPIO_NUM_11

// Thermocouples
#define TC_SCK_PIN              GPIO_NUM_38
#define TC_MISO_PIN             GPIO_NUM_39
#define TC_nCS1_PIN             40
#define TC_nCS2_PIN             41
#define TC_nCS3_PIN             42

// CC driver
#define CC_PWM_PIN              GPIO_NUM_4

// H-Bridge CH 0
#define H_BRIDGE_0_PWM1_PIN     GPIO_NUM_16
#define H_BRIDGE_0_PWM2_PIN     GPIO_NUM_15

// H-Bridge CH 1
#define H_BRIDGE_1_PWM1_PIN     GPIO_NUM_18
#define H_BRIDGE_1_PWM2_PIN     GPIO_NUM_17

// Power switch
#define PS_PWM_1_PIN            GPIO_NUM_7
#define PS_PWM_2_PIN            GPIO_NUM_6
#define PS_PWM_3_PIN            GPIO_NUM_5


