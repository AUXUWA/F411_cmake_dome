#include "lcd_anim.h"

/* --- 私有常量：标准立方体顶点 (归一化坐标 -1 到 1) --- */
static const Point3D cube_vertices[8] = {
    {-1, -1, -1}, { 1, -1, -1}, { 1,  1, -1}, {-1,  1, -1},
    {-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1}, {-1,  1,  1}
};

/* --- 私有常量：12条边 (连接顶点的索引) --- */
static const uint8_t cube_edges[12][2] = {
    {0,1}, {1,2}, {2,3}, {3,0}, // 前面
    {4,5}, {5,6}, {6,7}, {7,4}, // 后面
    {0,4}, {1,5}, {2,6}, {3,7}  // 中间连接
};

/* --- 私有函数：3D 投影到 2D --- */
static void _project_point(Point3D p, Point2D *p2d, float size, 
                           float angX, float angY, float angZ, 
                           int16_t cx, int16_t cy) 
{
    float x = p.x * size;
    float y = p.y * size;
    float z = p.z * size;
    float temp;

    // 1. 绕 X 轴旋转
    temp = y; 
    y = y * cosf(angX) - z * sinf(angX); 
    z = temp * sinf(angX) + z * cosf(angX);

    // 2. 绕 Y 轴旋转
    temp = x; 
    x = x * cosf(angY) + z * sinf(angY); 
    z = -temp * sinf(angY) + z * cosf(angY);

    // 3. 绕 Z 轴旋转
    temp = x; 
    x = x * cosf(angZ) - y * sinf(angZ); 
    y = temp * sinf(angZ) + y * cosf(angZ);

    // 4. 透视投影 (Perspective Projection)
    // 200.0f 是视距因子，z + 150.0f 是为了将物体推远，防止除零
    float factor = 200.0f / (z + 150.0f);
    
    // 5. 映射到屏幕坐标 (应用 cx, cy 偏移)
    p2d->x = (int16_t)(x * factor) + cx;
    p2d->y = (int16_t)(y * factor) + cy; 
}

/* --- 私有函数：画线框 (带边界检查) --- */
static void _draw_wireframe(lcd* plcd, Point2D* points, uint16_t color) {
    for (int i = 0; i < 12; i++) {
        Point2D p1 = points[cube_edges[i][0]];
        Point2D p2 = points[cube_edges[i][1]];
        
        // 简单保护：如果点在屏幕外太远，就不画，防止内存溢出或死机
        // 假设屏幕最大 320x240，留一点余量
        if (p1.x < -50 || p1.x > 400 || p1.y < -50 || p1.y > 400) continue;
        if (p2.x < -50 || p2.x > 400 || p2.y < -50 || p2.y > 400) continue;

        lcd_draw_line(plcd, p1.x, p1.y, p2.x, p2.y, color);
    }
}

/* --- 公开接口实现 --- */

void lcd_anim_cube_init(lcd_anim_cube_t* anim, lcd* plcd, float size, uint16_t color, int16_t x, int16_t y)
{
    // 参数绑定
    anim->lcd_handle = plcd;
    anim->size = size;
    anim->color = color;
    anim->cx = x;
    anim->cy = y;
    
    // 默认初始状态
    anim->speed = 0.05f; // 约 2.8 度每帧
    anim->angX = 0;
    anim->angY = 0;
    anim->angZ = 0;
    anim->is_first_frame = 1; // 必须置1，否则第一帧擦除时会出错
}

void lcd_anim_cube_run(lcd_anim_cube_t* anim)
{
    Point2D curr_points[8];
    
    // 1. 更新角度
    anim->angX += anim->speed;
    anim->angY += anim->speed * 0.6f;
    anim->angZ += anim->speed * 0.3f;

    // 2. 计算所有 8 个顶点的新坐标
    for(int i=0; i<8; i++) {
        _project_point(cube_vertices[i], &curr_points[i], anim->size,
                       anim->angX, anim->angY, anim->angZ,
                       anim->cx, anim->cy);
    }

    // 3. 擦除上一帧 (策略：用背景色重画一遍旧线)
    // 假设背景是黑色 (BLACK)。如果你的背景是其他颜色，请修改这里。
    if (!anim->is_first_frame) {
        _draw_wireframe(anim->lcd_handle, anim->prev_points, BLACK);
    }

    // 4. 绘制当前帧 (用指定颜色画新线)
    _draw_wireframe(anim->lcd_handle, curr_points, anim->color);

    // 5. 保存当前状态供下一帧擦除使用
    for(int i=0; i<8; i++) {
        anim->prev_points[i] = curr_points[i];
    }
    anim->is_first_frame = 0;
}