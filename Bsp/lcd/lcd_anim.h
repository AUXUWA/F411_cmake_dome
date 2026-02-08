#ifndef __LCD_ANIM_H__
#define __LCD_ANIM_H__

#include "lcd.h"
#include <math.h>
#include <string.h>

/* --- 配置参数 --- */
#define LCD_WIDTH   240
#define LCD_HEIGHT  135

/* --- 数据结构 --- */
typedef struct { float x, y, z; } Point3D;
typedef struct { int16_t x, y; } Point2D;

/* 立方体动画对象 */
typedef struct {
    // 关联的 LCD 句柄
    lcd* lcd_handle;
    
    // 属性
    float size;         // 大小
    uint16_t color;     // 颜色
    int16_t cx, cy;     // 屏幕位置 (Center X, Y)
    float speed;        // 旋转速度

    // 内部状态
    float angX, angY, angZ;
} lcd_anim_cube_t;

/* --- 函数接口 --- */

/**
 * @brief 初始化动画模块 (分配/清空显存)
 */
void lcd_anim_init_buffer(void);

/**
 * @brief 初始化一个立方体
 */
void lcd_anim_cube_init(lcd_anim_cube_t* anim, lcd* plcd, float size, uint16_t color, int16_t x, int16_t y);

/**
 * @brief 核心函数：
 * 1. 清空显存
 * 2. 计算并绘制立方体到显存
 */
void lcd_anim_cube_update(lcd_anim_cube_t* anim);
void lcd_print_ram(lcd* plcd, uint16_t x, uint16_t y, const char *fmt, ...);
void lcd_anim_flush(lcd* plcd);

#endif