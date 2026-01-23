#ifndef __XL9555_H
#define __XL9555_H

#include <stdint.h>
#include "esp_err.h"

/* XL9555 I2C 地址 */
#define XL9555_ADDR         0x20

/* I2C 引脚定义 */
#define I2C_SCL_PIN         42
#define I2C_SDA_PIN         41

/* XL9555 寄存器地址 */
#define XL9555_INPUT_PORT0_REG      0
#define XL9555_INPUT_PORT1_REG      1
#define XL9555_OUTPUT_PORT0_REG     2
#define XL9555_OUTPUT_PORT1_REG     3
#define XL9555_CONFIG_PORT0_REG     6
#define XL9555_CONFIG_PORT1_REG     7

/* XL9555 引脚定义 */
#define XL9555_P00          0x0001
#define XL9555_P01          0x0002
#define XL9555_P02          0x0004
#define XL9555_P03          0x0008
#define XL9555_P04          0x0010
#define XL9555_P05          0x0020
#define XL9555_P06          0x0040
#define XL9555_P07          0x0080
#define XL9555_P10          0x0100
#define XL9555_P11          0x0200
#define XL9555_P12          0x0400
#define XL9555_P13          0x0800
#define XL9555_P14          0x1000
#define XL9555_P15          0x2000
#define XL9555_P16          0x4000
#define XL9555_P17          0x8000

/* LCD 相关引脚定义 */
#define SLCD_PWR            XL9555_P13  /* LCD背光控制 */
#define SLCD_RST            XL9555_P12  /* LCD复位 */

/* IO 模式 */
typedef enum {
    IO_MODE_OUTPUT = 0,
    IO_MODE_INPUT = 1
} xl9555_io_mode_t;

/* IO 电平 */
typedef enum {
    IO_LOW = 0,
    IO_HIGH = 1
} xl9555_io_level_t;

/**
 * @brief 初始化XL9555
 * @return ESP_OK成功，其他失败
 */
esp_err_t xl9555_init(void);

/**
 * @brief 配置XL9555某个IO的模式
 * @param pin IO引脚编号
 * @param mode IO模式（输入/输出）
 * @return ESP_OK成功，其他失败
 */
esp_err_t xl9555_pin_config(uint16_t pin, xl9555_io_mode_t mode);

/**
 * @brief 设置XL9555某个IO的电平
 * @param pin IO引脚编号
 * @param level 电平（高/低）
 * @return ESP_OK成功，其他失败
 */
esp_err_t xl9555_pin_set(uint16_t pin, xl9555_io_level_t level);

/**
 * @brief 读取XL9555某个IO的电平
 * @param pin IO引脚编号
 * @param level 读取到的电平
 * @return ESP_OK成功，其他失败
 */
esp_err_t xl9555_pin_read(uint16_t pin, xl9555_io_level_t *level);

#endif // __XL9555_H
