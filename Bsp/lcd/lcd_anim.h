#ifndef __LCD_ANIM_H__
#define __LCD_ANIM_H__

#include "lcd.h"   // 确保包含了你的 lcd 驱动头文件
#include <math.h>
#include <stdint.h>

/* --- 数据结构定义 --- */

// 3D 点 (浮点数用于计算)
typedef struct { float x, y, z; } Point3D;

// 2D 点 (整数用于屏幕坐标)
typedef struct { int16_t x, y; } Point2D;

// 立方体动画句柄
typedef struct {
    // 1. 关联对象
    lcd* lcd_handle;    // 指向 LCD 驱动的指针

    // 2. 外观参数 (初始化时设置)
    float size;         // 立方体大小 (例如 30.0)
    uint16_t color;     // 线条颜色
    int16_t cx;         // 中心位置 X (Center X)
    int16_t cy;         // 中心位置 Y (Center Y)
    float speed;        // 旋转速度 (默认 0.05)

    // 3. 内部状态 (运行时自动更新，无需手动修改)
    float angX, angY, angZ; // 当前三轴旋转角度
    Point2D prev_points[8]; // 保存上一帧的 2D 坐标 (用于擦除)
    uint8_t is_first_frame; // 标记是否为第一帧
} lcd_anim_cube_t;

/* --- 函数原型 --- */

/**
 * @brief 初始化一个立方体动画对象
 * @param anim  动画句柄指针
 * @param plcd  LCD 驱动句柄
 * @param size  立方体边长的一半 (推荐 20~40)
 * @param color 线条颜色
 * @param x     屏幕中心 X 坐标
 * @param y     屏幕中心 Y 坐标
 */
void lcd_anim_cube_init(lcd_anim_cube_t* anim, lcd* plcd, float size, uint16_t color, int16_t x, int16_t y);

/**
 * @brief 运行一帧动画 (计算 -> 擦除旧线 -> 绘制新线)
 * @note  请在 while(1) 循环中调用
 */
void lcd_anim_cube_run(lcd_anim_cube_t* anim);

#endif /* __LCD_ANIM_H__ */