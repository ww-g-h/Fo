#ifndef __SPILCD_H
#define __SPILCD_H

#include <stdint.h>
#include "esp_err.h"

/* LCD尺寸定义 */
#define LCD_WIDTH           240
#define LCD_HEIGHT          320

/* LCD GPIO引脚定义 */
#define LCD_SCK_PIN         12
#define LCD_MOSI_PIN        11
#define LCD_CS_PIN          21
#define LCD_DC_PIN          40

/* LCD颜色定义 (RGB565格式) */
#define WHITE               0xFFFF
#define BLACK               0x0000
#define BLUE                0x001F
#define RED                 0xF800
#define MAGENTA             0xF81F
#define GREEN               0x07E0
#define CYAN                0x7FFF
#define YELLOW              0xFFE0
#define BROWN               0XBC40
#define BRRED               0XFC07
#define GRAY                0X8430
#define DARKBLUE            0X01CF
#define LIGHTBLUE           0X7D7C
#define GRAYBLUE            0X5458
#define LIGHTGREEN          0X841F
#define LGRAY               0XC618
#define LGRAYBLUE           0XA651
#define LBBLUE              0X2B12

/**
 * @brief 初始化LCD
 * @return ESP_OK成功，其他失败
 */
esp_err_t lcd_init(void);

/**
 * @brief 设置LCD绘图区域
 * @param x1 起始X坐标
 * @param y1 起始Y坐标
 * @param x2 结束X坐标
 * @param y2 结束Y坐标
 */
void lcd_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

/**
 * @brief 清屏
 * @param color 清屏颜色
 */
void lcd_clear(uint16_t color);

/**
 * @brief 填充矩形区域
 * @param x1 起始X坐标
 * @param y1 起始Y坐标
 * @param x2 结束X坐标
 * @param y2 结束Y坐标
 * @param color 填充颜色
 */
void lcd_fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

/**
 * @brief 画点
 * @param x X坐标
 * @param y Y坐标
 * @param color 颜色
 */
void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color);

/**
 * @brief 显示字符串
 * @param x X坐标
 * @param y Y坐标
 * @param str 字符串
 * @param font_size 字体大小 (16/24/32)
 * @param color 字体颜色
 * @param bg_color 背景颜色
 */
void lcd_show_string(uint16_t x, uint16_t y, const char *str, uint8_t font_size, uint16_t color, uint16_t bg_color);

/**
 * @brief 显示图标
 * @param x X坐标
 * @param y Y坐标
 * @param width 宽度
 * @param height 高度
 * @param image 图像数据指针
 */
void lcd_show_image(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *image);

/**
 * @brief 开启LCD背光
 */
void lcd_backlight_on(void);

/**
 * @brief 关闭LCD背光
 */
void lcd_backlight_off(void);

#endif // __SPILCD_H
