#ifndef __RGB_H
#define __RGB_H

#include "main.h"

// 颜色枚举
typedef enum {
    RGB_RED     = 0,
    RGB_GREEN   = 1,
    RGB_BLUE    = 2,
    RGB_YELLOW  = 3, // 红+绿
    RGB_PURPLE  = 4, // 红+蓝
    RGB_CYAN    = 5, // 绿+蓝
    RGB_WHITE   = 6, // 红+绿+蓝
    RGB_OFF     = 7
} RGB_Color_t;

#define LED_R(n) HAL_GPIO_WritePin(RED_GPIO_Port, RED_Pin, n ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define LED_G(n) HAL_GPIO_WritePin(GREEN_GPIO_Port, GREEN_Pin, n ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define LED_B(n) HAL_GPIO_WritePin(BLUE_GPIO_Port, BLUE_Pin, n ? GPIO_PIN_SET : GPIO_PIN_RESET)

void RGB_SetColor(uint8_t color);

#endif