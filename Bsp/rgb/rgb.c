#include "RGB.h"
#include "gpio.h"

/**
 * @brief 设置RGB灯颜色
 * @param color 颜色索引 (0-6)
 */
void RGB_SetColor(uint8_t color)
{
    // 0 为点亮(Low)，1 为熄灭(High) —— 共阳极接法
    switch(color % 7)
    {
        case 0: // 红色
            LED_R(0); LED_G(1); LED_B(1);
            break;
        case 1: // 绿色
            LED_R(1); LED_G(0); LED_B(1);
            break;
        case 2: // 蓝色
            LED_R(1); LED_G(1); LED_B(0);
            break;
        case 3: // 黄色 (红+绿)
            LED_R(0); LED_G(0); LED_B(1);
            break;
        case 4: // 品红 (红+蓝)
            LED_R(0); LED_G(1); LED_B(0);
            break;
        case 5: // 青色 (绿+蓝)
            LED_R(1); LED_G(0); LED_B(0);
            break;
        case 6: // 白色 (全亮)
            LED_R(0); LED_G(0); LED_B(0);
            break;
        default: // 关闭
            LED_R(1); LED_G(1); LED_B(1);
            break;
    }
}