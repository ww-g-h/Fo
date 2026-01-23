#include "xl9555.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "XL9555";

static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static i2c_master_dev_handle_t xl9555_dev_handle = NULL;

static uint8_t port0_output = 0xFF;  // 默认输出高电平
static uint8_t port1_output = 0xFF;

/**
 * @brief 写XL9555寄存器
 */
static esp_err_t xl9555_write_reg(uint8_t reg, uint8_t data)
{
    uint8_t write_buf[2] = {reg, data};
    return i2c_master_transmit(xl9555_dev_handle, write_buf, 2, -1);
}

/**
 * @brief 读XL9555寄存器
 */
static esp_err_t xl9555_read_reg(uint8_t reg, uint8_t *data)
{
    esp_err_t ret;
    ret = i2c_master_transmit(xl9555_dev_handle, &reg, 1, -1);
    if (ret != ESP_OK) {
        return ret;
    }
    return i2c_master_receive(xl9555_dev_handle, data, 1, -1);
}

/**
 * @brief 初始化XL9555
 */
esp_err_t xl9555_init(void)
{
    esp_err_t ret;

    // 配置I2C总线
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_SCL_PIN,
        .sda_io_num = I2C_SDA_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ret = i2c_new_master_bus(&bus_config, &i2c_bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C master bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 添加XL9555设备
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = XL9555_ADDR,
        .scl_speed_hz = 400000,  // 400kHz
    };

    ret = i2c_master_bus_add_device(i2c_bus_handle, &dev_config, &xl9555_dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add XL9555 device: %s", esp_err_to_name(ret));
        return ret;
    }

    // 默认配置所有IO为输出
    xl9555_write_reg(XL9555_CONFIG_PORT0_REG, 0x00);
    xl9555_write_reg(XL9555_CONFIG_PORT1_REG, 0x00);

    // 设置默认输出电平
    xl9555_write_reg(XL9555_OUTPUT_PORT0_REG, port0_output);
    xl9555_write_reg(XL9555_OUTPUT_PORT1_REG, port1_output);

    ESP_LOGI(TAG, "XL9555 initialized successfully");
    return ESP_OK;
}

/**
 * @brief 配置XL9555某个IO的模式
 */
esp_err_t xl9555_pin_config(uint16_t pin, xl9555_io_mode_t mode)
{
    uint8_t reg, config_value;
    esp_err_t ret;

    // 判断是PORT0还是PORT1
    if (pin <= XL9555_P07) {
        reg = XL9555_CONFIG_PORT0_REG;
        ret = xl9555_read_reg(reg, &config_value);
        if (ret != ESP_OK) return ret;

        if (mode == IO_MODE_OUTPUT) {
            config_value &= ~pin;
        } else {
            config_value |= pin;
        }
    } else {
        reg = XL9555_CONFIG_PORT1_REG;
        ret = xl9555_read_reg(reg, &config_value);
        if (ret != ESP_OK) return ret;

        if (mode == IO_MODE_OUTPUT) {
            config_value &= ~(pin >> 8);
        } else {
            config_value |= (pin >> 8);
        }
    }

    return xl9555_write_reg(reg, config_value);
}

/**
 * @brief 设置XL9555某个IO的电平
 */
esp_err_t xl9555_pin_set(uint16_t pin, xl9555_io_level_t level)
{
    uint8_t reg;
    uint8_t *output_val;

    // 判断是PORT0还是PORT1
    if (pin <= XL9555_P07) {
        reg = XL9555_OUTPUT_PORT0_REG;
        output_val = &port0_output;
        
        if (level == IO_HIGH) {
            *output_val |= pin;
        } else {
            *output_val &= ~pin;
        }
    } else {
        reg = XL9555_OUTPUT_PORT1_REG;
        output_val = &port1_output;
        
        if (level == IO_HIGH) {
            *output_val |= (pin >> 8);
        } else {
            *output_val &= ~(pin >> 8);
        }
    }

    return xl9555_write_reg(reg, *output_val);
}

/**
 * @brief 读取XL9555某个IO的电平
 */
esp_err_t xl9555_pin_read(uint16_t pin, xl9555_io_level_t *level)
{
    uint8_t reg, input_value;
    esp_err_t ret;

    // 判断是PORT0还是PORT1
    if (pin <= XL9555_P07) {
        reg = XL9555_INPUT_PORT0_REG;
        ret = xl9555_read_reg(reg, &input_value);
        if (ret != ESP_OK) return ret;
        
        *level = (input_value & pin) ? IO_HIGH : IO_LOW;
    } else {
        reg = XL9555_INPUT_PORT1_REG;
        ret = xl9555_read_reg(reg, &input_value);
        if (ret != ESP_OK) return ret;
        
        *level = (input_value & (pin >> 8)) ? IO_HIGH : IO_LOW;
    }

    return ESP_OK;
}
