#include "spilcd.h"
#include "xl9555.h"
#include "font.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "LCD";

static spi_device_handle_t spi_handle = NULL;

#define LCD_TX_CHUNK_PIXELS 64

/* LCD命令/数据控制宏 */
#define LCD_CMD()   gpio_set_level(LCD_DC_PIN, 0)
#define LCD_DATA()  gpio_set_level(LCD_DC_PIN, 1)

/**
 * @brief 写命令到LCD
 */
static void lcd_write_cmd(uint8_t cmd)
{
    LCD_CMD();
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    spi_device_polling_transmit(spi_handle, &t);
}

/**
 * @brief 写数据到LCD
 */
static void lcd_write_data(uint8_t data)
{
    LCD_DATA();
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &data,
    };
    spi_device_polling_transmit(spi_handle, &t);
}

/**
 * @brief 写批量数据到LCD
 */
static void lcd_write_data_batch(const uint8_t *data, size_t len)
{
    LCD_DATA();
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    spi_device_polling_transmit(spi_handle, &t);
}

/**
 * @brief 写16位数据到LCD
 */
static void lcd_write_data16(uint16_t data)
{
    uint8_t buf[2];
    buf[0] = data >> 8;
    buf[1] = data & 0xFF;
    lcd_write_data_batch(buf, 2);
}

/**
 * @brief 设置LCD绘图区域
 */
void lcd_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    lcd_write_cmd(0x2A);  // 列地址设置
    lcd_write_data(x1 >> 8);
    lcd_write_data(x1 & 0xFF);
    lcd_write_data(x2 >> 8);
    lcd_write_data(x2 & 0xFF);

    lcd_write_cmd(0x2B);  // 行地址设置
    lcd_write_data(y1 >> 8);
    lcd_write_data(y1 & 0xFF);
    lcd_write_data(y2 >> 8);
    lcd_write_data(y2 & 0xFF);

    lcd_write_cmd(0x2C);  // 写内存
}

/**
 * @brief 画点
 */
void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_set_window(x, y, x, y);
    lcd_write_data16(color);
}

/**
 * @brief 填充矩形区域
 */
void lcd_fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint32_t num = (x2 - x1 + 1) * (y2 - y1 + 1);
    uint16_t color_be = (color >> 8) | (color << 8);  // 转换为大端序
    uint16_t buf[LCD_TX_CHUNK_PIXELS];

    for (int i = 0; i < LCD_TX_CHUNK_PIXELS; i++) {
        buf[i] = color_be;
    }

    lcd_set_window(x1, y1, x2, y2);

    LCD_DATA();
    uint32_t sent = 0;
    uint32_t batch_count = 0;
    while (sent < num) {
        uint32_t batch = num - sent;
        if (batch > LCD_TX_CHUNK_PIXELS) {
            batch = LCD_TX_CHUNK_PIXELS;
        }
        spi_transaction_t t = {
            .length = batch * 16,
            .tx_buffer = buf,
        };
        spi_device_polling_transmit(spi_handle, &t);
        sent += batch;

        if ((++batch_count & 0x1F) == 0) {
            vTaskDelay(1);
        }
    }
}

/**
 * @brief 清屏
 */
void lcd_clear(uint16_t color)
{
    lcd_fill(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, color);
}

/**
 * @brief 显示ASCII字符
 */
static void lcd_show_char(uint16_t x, uint16_t y, char ch, uint8_t font_size, uint16_t color, uint16_t bg_color)
{
    uint8_t temp, t;
    uint16_t y0 = y;
    uint8_t csize = (font_size / 8 + ((font_size % 8) ? 1 : 0)) * (font_size / 2);
    uint8_t ch_index = (uint8_t)ch;
    uint32_t pixel_count = 0;

    if (ch_index < ' ' || ch_index > '~') {
        return;
    }

    ch_index = (uint8_t)(ch_index - ' ');  // 得到偏移后的值
    
    for (t = 0; t < csize; t++) {
        if (font_size == 16) {
            temp = font_ascii_16[ch_index][t];
        } else if (font_size == 24) {
            temp = font_ascii_24[ch_index][t];
        } else if (font_size == 32) {
            temp = font_ascii_32[ch_index][t];
        } else {
            return;
        }
        
        for (uint8_t i = 0; i < 8; i++) {
            if (temp & 0x80) {
                lcd_draw_point(x, y, color);
            } else {
                lcd_draw_point(x, y, bg_color);
            }
            temp <<= 1;
            y++;
            if ((++pixel_count & 0x3F) == 0) {
                vTaskDelay(1);
            }
            if ((y - y0) == font_size) {
                y = y0;
                x++;
                break;
            }
        }
    }
}

/**
 * @brief 显示字符串
 */
void lcd_show_string(uint16_t x, uint16_t y, const char *str, uint8_t font_size, uint16_t color, uint16_t bg_color)
{
    uint16_t x0 = x;
    uint32_t char_count = 0;
    
    while (*str != 0) {
        if (*str == '\n') {
            y += font_size;
            x = x0;
        } else if (*str == '\r') {
            x = x0;
        } else {
            if (x > LCD_WIDTH - font_size / 2) {
                x = x0;
                y += font_size;
            }
            if (y > LCD_HEIGHT - font_size) {
                y = x = 0;
                lcd_clear(bg_color);
            }
            lcd_show_char(x, y, *str, font_size, color, bg_color);
            x += font_size / 2;
            if ((++char_count & 0x07) == 0) {
                vTaskDelay(1);
            }
        }
        str++;
    }
}

/**
 * @brief 显示图标/图像
 */
void lcd_show_image(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *image)
{
    uint32_t num = width * height;
    uint16_t buf[LCD_TX_CHUNK_PIXELS];

    lcd_set_window(x, y, x + width - 1, y + height - 1);

    LCD_DATA();

    uint32_t sent = 0;
    uint32_t batch_count = 0;
    while (sent < num) {
        uint32_t batch = num - sent;
        if (batch > LCD_TX_CHUNK_PIXELS) {
            batch = LCD_TX_CHUNK_PIXELS;
        }
        for (uint32_t i = 0; i < batch; i++) {
            uint16_t color = image[sent + i];
            buf[i] = (color >> 8) | (color << 8);
        }
        spi_transaction_t t = {
            .length = batch * 16,
            .tx_buffer = buf,
        };
        spi_device_polling_transmit(spi_handle, &t);
        sent += batch;

        if ((++batch_count & 0x1F) == 0) {
            vTaskDelay(1);
        }
    }
}

/**
 * @brief 开启LCD背光
 */
void lcd_backlight_on(void)
{
    xl9555_pin_set(SLCD_PWR, IO_HIGH);
}

/**
 * @brief 关闭LCD背光
 */
void lcd_backlight_off(void)
{
    xl9555_pin_set(SLCD_PWR, IO_LOW);
}

/**
 * @brief 初始化LCD
 */
esp_err_t lcd_init(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing LCD...");

    // 配置GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LCD_DC_PIN) | (1ULL << LCD_CS_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // 配置XL9555控制的引脚
    xl9555_pin_config(SLCD_PWR, IO_MODE_OUTPUT);
    xl9555_pin_config(SLCD_RST, IO_MODE_OUTPUT);
    
    // 默认电平
    xl9555_pin_set(SLCD_PWR, IO_HIGH);
    xl9555_pin_set(SLCD_RST, IO_HIGH);
    gpio_set_level(LCD_DC_PIN, 1);
    gpio_set_level(LCD_CS_PIN, 1);

    // 配置SPI总线
    spi_bus_config_t buscfg = {
        .mosi_io_num = LCD_MOSI_PIN,
        .miso_io_num = -1,
        .sclk_io_num = LCD_SCK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * 2,
    };

    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 添加SPI设备
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 40 * 1000 * 1000,  // 40MHz
        .mode = 3,                            // SPI mode 3
        .spics_io_num = LCD_CS_PIN,
        .queue_size = 7,
        .flags = SPI_DEVICE_NO_DUMMY,
    };

    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        return ret;
    }

    // 硬件复位
    xl9555_pin_set(SLCD_RST, IO_HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    xl9555_pin_set(SLCD_RST, IO_LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    xl9555_pin_set(SLCD_RST, IO_HIGH);
    vTaskDelay(pdMS_TO_TICKS(120));

    // ST7789V 初始化序列
    lcd_write_cmd(0x11);  // Sleep Out
    vTaskDelay(pdMS_TO_TICKS(120));

    lcd_write_cmd(0x3A);  // Interface Pixel Format
    lcd_write_data(0x05);  // 16bit/pixel

    lcd_write_cmd(0xC5);  // VCOM
    lcd_write_data(0x1A);

    lcd_write_cmd(0x36);  // Memory Data Access Control
    lcd_write_data(0x00);

    lcd_write_cmd(0xB2);  // Porch Setting
    lcd_write_data(0x05);
    lcd_write_data(0x05);
    lcd_write_data(0x00);
    lcd_write_data(0x33);
    lcd_write_data(0x33);

    lcd_write_cmd(0xB7);  // Gate Control
    lcd_write_data(0x05);

    lcd_write_cmd(0xBB);  // VCOM
    lcd_write_data(0x3F);

    lcd_write_cmd(0xC0);  // Power control
    lcd_write_data(0x2c);

    lcd_write_cmd(0xC2);  // VDV and VRH Command Enable
    lcd_write_data(0x01);

    lcd_write_cmd(0xC3);  // VRH Set
    lcd_write_data(0x0F);

    lcd_write_cmd(0xC4);  // VDV Set
    lcd_write_data(0x20);

    lcd_write_cmd(0xC6);  // Frame Rate Control
    lcd_write_data(0x01);  // 111Hz

    lcd_write_cmd(0xD0);  // Power Control 1
    lcd_write_data(0xA4);
    lcd_write_data(0xA1);

    lcd_write_cmd(0xE8);
    lcd_write_data(0x03);

    lcd_write_cmd(0xE9);
    lcd_write_data(0x09);
    lcd_write_data(0x09);
    lcd_write_data(0x08);

    // Gamma设置
    lcd_write_cmd(0xE0);
    lcd_write_data(0xD0);
    lcd_write_data(0x05);
    lcd_write_data(0x09);
    lcd_write_data(0x09);
    lcd_write_data(0x08);
    lcd_write_data(0x14);
    lcd_write_data(0x28);
    lcd_write_data(0x33);
    lcd_write_data(0x3F);
    lcd_write_data(0x07);
    lcd_write_data(0x13);
    lcd_write_data(0x14);
    lcd_write_data(0x28);
    lcd_write_data(0x30);

    lcd_write_cmd(0xE1);
    lcd_write_data(0xD0);
    lcd_write_data(0x05);
    lcd_write_data(0x09);
    lcd_write_data(0x09);
    lcd_write_data(0x08);
    lcd_write_data(0x03);
    lcd_write_data(0x24);
    lcd_write_data(0x32);
    lcd_write_data(0x32);
    lcd_write_data(0x3B);
    lcd_write_data(0x14);
    lcd_write_data(0x13);
    lcd_write_data(0x28);
    lcd_write_data(0x2F);

    lcd_write_cmd(0x21);  // Display Inversion On
    lcd_write_cmd(0x29);  // Display On

    // 开启背光
    lcd_backlight_on();

    // 清屏
    lcd_clear(BLACK);

    ESP_LOGI(TAG, "LCD initialized successfully");
    return ESP_OK;
}
