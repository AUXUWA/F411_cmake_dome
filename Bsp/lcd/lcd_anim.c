#include "lcd_anim.h"
#include <stdarg.h> 
#include <stdio.h>

/* --- Framebuffer --- */
// 240 * 135 * 2 Bytes = 64,800 Bytes
uint16_t g_gram[LCD_WIDTH * LCD_HEIGHT];

static const Point3D cube_vertices[8] = {
    {-1, -1, -1}, { 1, -1, -1}, { 1,  1, -1}, {-1,  1, -1},
    {-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1}, {-1,  1,  1}
};

static const uint8_t cube_edges[12][2] = {
    {0,1}, {1,2}, {2,3}, {3,0}, // 前面
    {4,5}, {5,6}, {6,7}, {7,4}, // 后面
    {0,4}, {1,5}, {2,6}, {3,7}  // 连接线
};

/* --- RAM 画点 (极快) --- */
static void _draw_point_ram(int16_t x, int16_t y, uint16_t color)
{
    if (x < 0 || x >= LCD_WIDTH || y < 0 || y >= LCD_HEIGHT) return;
    
    // 写入数组，注意：这里为了DMA传输效率，建议直接存交换后的大小端
    // 如果你的 lcd_write_bulk 内部没有交换大小端，这里需要 color = (color<<8)|(color>>8);
    // 假设 lcd_write_bulk 是纯数据发送，我们在存的时候就交换好：
    g_gram[y * LCD_WIDTH + x] = (color << 8) | (color >> 8);
}

/* --- RAM 画线 (Bresenham算法) --- */
static void _draw_line_ram(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
    int xerr = 0, yerr = 0;
    int delta_x = x2 - x1, delta_y = y2 - y1;
    int distance;
    int incx, incy, uRow, uCol;

    if (delta_x > 0) incx = 1;
    else if (delta_x == 0) incx = 0;
    else { incx = -1; delta_x = -delta_x; }

    if (delta_y > 0) incy = 1;
    else if (delta_y == 0) incy = 0;
    else { incy = -1; delta_y = -delta_y; }

    uRow = x1; uCol = y1;
    distance = delta_x > delta_y ? delta_x : delta_y;

    for (int t = 0; t <= distance + 1; t++) {
        _draw_point_ram(uRow, uCol, color); // <--- 画到 RAM
        xerr += delta_x;
        yerr += delta_y;
        if (xerr > distance) { xerr -= distance; uRow += incx; }
        if (yerr > distance) { yerr -= distance; uCol += incy; }
    }
}

/**
 * @brief 在 RAM 中显示一个字符
 */
void lcd_show_char_ram(lcd* plcd, uint16_t x, uint16_t y, uint16_t chr)
{
    uint8_t width_cnt = 0; // 当前行的像素计数
    uint8_t y_offset = 0;  // 当前行的 Y 偏移
    
    // 1. 基础边界检查
    // 如果起始位置已经超出屏幕，直接返回
    if(x >= plcd->hw->width || y >= plcd->hw->height) return;

    // 2. 字符偏移计算
    chr = chr - ' '; 
    
    // 3. 遍历字库数据的每一个字节
    for(int idx = 0; idx < plcd->font.bytes; idx++) 
    {
        // 获取当前字节数据
        uint8_t data = plcd->font.addr[chr * plcd->font.bytes + idx];
        
        // 遍历字节中的 8 个位 (像素)
        for(int pixel = 0; pixel < 8; pixel++) 
        {
            // 计算当前像素的绝对坐标
            int16_t draw_x = x + width_cnt;
            int16_t draw_y = y + y_offset;

            // 二次边界检查：确保像素没画出屏幕外
            if (draw_x < plcd->hw->width && draw_y < plcd->hw->height) 
            {
                uint32_t ram_addr = draw_y * plcd->hw->width + draw_x;
                uint16_t color_to_write;

                // 判断该位是 0 还是 1
                if(data & 0x01) {
                    color_to_write = plcd->font.front_color; // 字体颜色
                } else {
                    color_to_write = plcd->font.back_color;  // 背景颜色
                }

                /* * 【重要】透明背景处理：
                 * 如果你只想要显示字，不想让字的黑色背景挡住后面的立方体，
                 * 请把下面的 else 分支注释掉！
                 */
                if (data & 0x01) {
                    // 大小端交换 (适配 DMA)
                    g_gram[ram_addr] = (color_to_write << 8) | (color_to_write >> 8);
                } else {
                    // 如果需要背景色，保留这行；如果想要透明，注释掉这行
                    g_gram[ram_addr] = (color_to_write << 8) | (color_to_write >> 8);
                }
            }

            // 数据移位，处理下一个像素
            data >>= 1;

            // 宽度计数增加
            width_cnt++;
            
            // 如果一行画完了 (例如 12号字体，width=6或8，这里取决于字库定义)
            if(width_cnt == plcd->font.width) {
                width_cnt = 0; // 重置 X 偏移
                y_offset++;    // 换行，Y 偏移 +1
                break;         // 跳出当前字节的循环，读取下一个字节
            }
        }
    }
}

/**
 * @brief 在 RAM 中显示一个字符串
 */
void lcd_show_string_ram(lcd* plcd, uint16_t x, uint16_t y, const char *p)
{
    while(*p != '\0') {
        // 自动换行检查 (可选)
        if(x > plcd->hw->width - plcd->font.width) {
            x = 0;
            y += plcd->font.height;
        }
        
        // 画字符
        lcd_show_char_ram(plcd, x, y, *p++); 
        
        // 移动光标
        x += plcd->font.width;
    }
}

/* --- 3D 投影 --- */
static void _project_point(Point3D p, Point2D *p2d, float size, 
                           float angX, float angY, float angZ, 
                           int16_t cx, int16_t cy) 
{
    float x = p.x * size;
    float y = p.y * size;
    float z = p.z * size;
    float temp;

    // 旋转运算
    temp = y; y = y * cosf(angX) - z * sinf(angX); z = temp * sinf(angX) + z * cosf(angX);
    temp = x; x = x * cosf(angY) + z * sinf(angY); z = -temp * sinf(angY) + z * cosf(angY);
    temp = x; x = x * cosf(angZ) - y * sinf(angZ); y = temp * sinf(angZ) + y * cosf(angZ);

    // 透视投影
    float factor = 200.0f / (z + 150.0f);
    p2d->x = (int16_t)(x * factor) + cx;
    p2d->y = (int16_t)(y * factor) + cy; 
}

/* --- 公开接口实现 --- */

void lcd_anim_init_buffer(void)
{
    // 初始化显存为全黑
    memset(g_gram, 0, sizeof(g_gram));
}

void lcd_anim_cube_init(lcd_anim_cube_t* anim, lcd* plcd, float size, uint16_t color, int16_t x, int16_t y)
{
    anim->lcd_handle = plcd;
    anim->size = size;
    anim->color = color;
    anim->cx = x;
    anim->cy = y;
    anim->speed = 0.05f;
    anim->angX = 0; anim->angY = 0; anim->angZ = 0;
}

void lcd_anim_cube_update(lcd_anim_cube_t* anim)
{
    Point2D p2d[8];
    
    // A. 显存清空 (非常重要，这就相当于擦除旧线)
    // 注意：如果有多个物体，memset 应该在主循环里调用一次，而不是每个物体调用一次
    // 这里为了演示方便，假设只有一个物体。
    // 如果你有多个物体，请在 task 里手动调 memset，并把这行注释掉
    // memset(g_gram, 0, sizeof(g_gram)); <--- 移到外面去更灵活

    // B. 更新角度
    anim->angX += anim->speed;
    anim->angY += anim->speed * 0.6f;
    anim->angZ += anim->speed * 0.3f;

    // C. 计算坐标
    for(int i=0; i<8; i++) {
        _project_point(cube_vertices[i], &p2d[i], anim->size,
                       anim->angX, anim->angY, anim->angZ,
                       anim->cx, anim->cy);
    }

    // D. 绘制线框到 RAM
    for (int i = 0; i < 12; i++) {
        Point2D p1 = p2d[cube_edges[i][0]];
        Point2D p2 = p2d[cube_edges[i][1]];
        _draw_line_ram(p1.x, p1.y, p2.x, p2.y, anim->color);
    }
}

/**
 * @brief 在 RAM 显存中格式化打印字符串
 * @param plcd  LCD 句柄 (包含字体信息)
 * @param x     起始 X 坐标
 * @param y     起始 Y 坐标
 * @param fmt   格式化字符串 (如 "FPS: %d")
 * @param ...   参数列表
 */
void lcd_print_ram(lcd* plcd, uint16_t x, uint16_t y, const char *fmt, ...)
{
    unsigned char buffer[128] = { 0 }; 
    va_list ap;
    
    va_start(ap, fmt);
    vsnprintf((char*)buffer, sizeof(buffer), fmt, ap);
    va_end(ap);

    lcd_show_string_ram(plcd, x, y, (const char*)buffer);
}

/**
 * @brief 将显存中的内容一次性推送到屏幕 (防撕裂关键)
 */
void lcd_anim_flush(lcd* plcd)
{
    // 1. 设置全屏窗口
    lcd_set_address(plcd, 0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    
    // 2. 发送显存数据
    // 注意：这里 g_gram 已经是交换过大小端的了
    lcd_write_bulk(plcd->io, (uint8_t*)g_gram, sizeof(g_gram));
}