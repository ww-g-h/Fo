#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#if CONFIG_USJ_ENABLE_USB_SERIAL_JTAG
#include "driver/usb_serial_jtag.h"
#endif
#include "esp_log.h"
#include "xl9555.h"
#include "spilcd.h"
#include "icons.h"

static const char *TAG = "MAIN";

/* UART配置 */
#define UART_NUM        UART_NUM_0
#define UART_TX_PIN     UART_PIN_NO_CHANGE
#define UART_RX_PIN     UART_PIN_NO_CHANGE
#define UART_BUF_SIZE   (1024)

/* 命令队列 */
static QueueHandle_t cmd_queue = NULL;

/* 命令结构体 */
typedef struct {
    char cmd[32];
} cmd_msg_t;

/**
 * @brief 统一处理并入队命令
 */
static void enqueue_command(const char *input)
{
    cmd_msg_t cmd_msg;
    char tmp[sizeof(cmd_msg.cmd)];
    size_t len = strnlen(input, sizeof(tmp) - 1);

    while (len > 0 && (input[len - 1] == '\r' || input[len - 1] == '\n' || input[len - 1] == ' ' || input[len - 1] == '\t')) {
        len--;
    }

    if (len == 0) {
        return;
    }

    for (size_t i = 0; i < len; i++) {
        tmp[i] = (char)tolower((unsigned char)input[i]);
    }
    tmp[len] = '\0';

    strncpy(cmd_msg.cmd, tmp, sizeof(cmd_msg.cmd) - 1);
    cmd_msg.cmd[sizeof(cmd_msg.cmd) - 1] = '\0';

    xQueueSend(cmd_queue, &cmd_msg, 0);
    ESP_LOGI(TAG, "Received command: %s", cmd_msg.cmd);
}

/**
 * @brief 解析一行命令（支持空格分隔的多个命令）
 */
static void enqueue_line(const char *line)
{
    const char *p = line;
    while (*p != '\0') {
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p == '\0') {
            break;
        }
        const char *start = p;
        while (*p != '\0' && *p != ' ' && *p != '\t') {
            p++;
        }
        if (p > start) {
            char token[32];
            size_t len = (size_t)(p - start);
            if (len >= sizeof(token)) {
                len = sizeof(token) - 1;
            }
            memcpy(token, start, len);
            token[len] = '\0';
            enqueue_command(token);
        }
    }
}

typedef void (*echo_fn_t)(const char *buf, size_t len);

static void process_input_char(char ch, char *line, size_t *line_len, size_t line_cap, echo_fn_t echo_fn)
{
    if (ch == '\r' || ch == '\n') {
        if (*line_len > 0) {
            line[*line_len] = '\0';
            enqueue_line(line);
            *line_len = 0;
        }
        if (echo_fn) {
            const char *nl = "\r\n";
            echo_fn(nl, 2);
        }
        return;
    }

    if (ch == 0x08 || ch == 0x7F) {
        if (*line_len > 0) {
            (*line_len)--;
            if (echo_fn) {
                const char *bs = "\b \b";
                echo_fn(bs, 3);
            }
        }
        return;
    }

    if (ch >= 0x20 && ch <= 0x7E) {
        if (*line_len < line_cap - 1) {
            line[(*line_len)++] = ch;
            if (echo_fn) {
                echo_fn(&ch, 1);
            }
        }
    }
}

static void echo_uart(const char *buf, size_t len)
{
    uart_write_bytes(UART_NUM, buf, len);
}

#if CONFIG_USJ_ENABLE_USB_SERIAL_JTAG
static void echo_usb(const char *buf, size_t len)
{
    usb_serial_jtag_write_bytes(buf, len, pdMS_TO_TICKS(10));
}
#endif

/**
 * @brief UART事件处理任务
 */
static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t *data = (uint8_t *)malloc(UART_BUF_SIZE + 1);
    char line[sizeof(((cmd_msg_t *)0)->cmd)];
    size_t line_len = 0;
    
    QueueHandle_t uart_queue;
    uart_driver_install(UART_NUM, UART_BUF_SIZE * 2, UART_BUF_SIZE * 2, 20, &uart_queue, 0);
    
    while (1) {
        if (xQueueReceive(uart_queue, (void *)&event, portMAX_DELAY)) {
            if (event.type == UART_DATA) {
                int len = uart_read_bytes(UART_NUM, data, event.size, portMAX_DELAY);
                if (len > 0) {
                    if (len > UART_BUF_SIZE) {
                        len = UART_BUF_SIZE;
                    }
                    for (int i = 0; i < len; i++) {
                        process_input_char((char)data[i], line, &line_len, sizeof(line), echo_uart);
                    }
                }
            }
        }
    }
    
    free(data);
    vTaskDelete(NULL);
}

#if CONFIG_USJ_ENABLE_USB_SERIAL_JTAG
/**
 * @brief USB Serial/JTAG 命令输入任务
 */
static void usb_serial_jtag_task(void *pvParameters)
{
    usb_serial_jtag_driver_config_t cfg = {
        .rx_buffer_size = UART_BUF_SIZE,
        .tx_buffer_size = UART_BUF_SIZE,
    };

    esp_err_t ret = usb_serial_jtag_driver_install(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "USB Serial/JTAG init failed: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }

    uint8_t buf[64];
    char line[sizeof(((cmd_msg_t *)0)->cmd)];
    size_t line_len = 0;

    while (1) {
        int len = usb_serial_jtag_read_bytes(buf, sizeof(buf), pdMS_TO_TICKS(100));
        if (len > 0) {
            for (int i = 0; i < len; i++) {
                process_input_char((char)buf[i], line, &line_len, sizeof(line), echo_usb);
            }
        }
    }
}
#endif

static void draw_filled_circle(uint16_t cx, uint16_t cy, uint16_t r, uint16_t color)
{
    uint32_t rr = (uint32_t)r * r;
    uint32_t count = 0;
    for (int y = -(int)r; y <= (int)r; y++) {
        for (int x = -(int)r; x <= (int)r; x++) {
            if ((uint32_t)(x * x + y * y) <= rr) {
                lcd_draw_point((uint16_t)(cx + x), (uint16_t)(cy + y), color);
            }
            if ((++count & 0x3FF) == 0) {
                vTaskDelay(1);
            }
        }
    }
}

static void draw_circle_arc(uint16_t cx, uint16_t cy, uint16_t r, float start_deg, float end_deg, uint16_t color)
{
    const float step = 2.0f;
    for (float deg = start_deg; deg <= end_deg; deg += step) {
        float rad = deg * (float)M_PI / 180.0f;
        int x = (int)(cosf(rad) * r);
        int y = (int)(sinf(rad) * r);
        lcd_draw_point((uint16_t)(cx + x), (uint16_t)(cy + y), color);
    }
}

static void draw_smile_face(void)
{
    lcd_clear(WHITE);
    draw_filled_circle(120, 140, 60, YELLOW);
    draw_filled_circle(98, 125, 6, BLACK);
    draw_filled_circle(142, 125, 6, BLACK);
    draw_circle_arc(120, 170, 28, 20.0f, 160.0f, BLACK);
    lcd_show_string(92, 220, "SMILE", 24, BLACK, WHITE);
}

static void draw_sad_face(void)
{
    lcd_clear(WHITE);
    draw_filled_circle(120, 140, 60, YELLOW);
    draw_filled_circle(98, 125, 6, BLACK);
    draw_filled_circle(142, 125, 6, BLACK);
    draw_circle_arc(120, 150, 28, 200.0f, 340.0f, BLACK);
    lcd_show_string(100, 220, "SAD", 24, BLACK, WHITE);
}

static void draw_wave_art(void)
{
    lcd_clear(WHITE);
    uint16_t x0 = 30;
    uint16_t y0 = 160;
    for (uint16_t x = 0; x < 180; x++) {
        float t = (float)x / 20.0f;
        uint16_t y = (uint16_t)(y0 + 20.0f * sinf(t));
        lcd_draw_point(x0 + x, y, BLUE);
        if ((x & 0x1F) == 0) {
            vTaskDelay(1);
        }
    }
    lcd_show_string(85, 90, "WAVE", 32, BLUE, WHITE);
}

/**
 * @brief 显示Hello World
 */
static void display_hello(void)
{
    lcd_clear(BLACK);
    lcd_show_string(60, 140, "Hello World!", 24, WHITE, BLACK);
    ESP_LOGI(TAG, "Displayed: Hello World!");
}

/**
 * @brief 显示Wave
 */
static void display_wave(void)
{
    draw_wave_art();
    ESP_LOGI(TAG, "Displayed: Wave icon");
}

/**
 * @brief 显示笑脸
 */
static void display_smile(void)
{
    draw_smile_face();
    ESP_LOGI(TAG, "Displayed: Smile icon");
}

/**
 * @brief 显示哭脸
 */
static void display_sad(void)
{
    draw_sad_face();
    ESP_LOGI(TAG, "Displayed: Sad icon");
}

/**
 * @brief 清空屏幕
 */
static void display_clear(void)
{
    lcd_clear(BLACK);
    ESP_LOGI(TAG, "Screen cleared");
}

/**
 * @brief 显示错误命令
 */
static void display_invalid(void)
{
    lcd_clear(RED);
    lcd_show_string(20, 140, "Invalid command", 24, WHITE, RED);
    ESP_LOGI(TAG, "Displayed: Invalid command");
}

/**
 * @brief 命令处理任务
 */
static void cmd_process_task(void *pvParameters)
{
    cmd_msg_t cmd_msg;
    
    while (1) {
        if (xQueueReceive(cmd_queue, &cmd_msg, portMAX_DELAY)) {
            ESP_LOGI(TAG, "Processing command: %s", cmd_msg.cmd);
            
            // 命令解析和处理
            if (strcmp(cmd_msg.cmd, "hello") == 0) {
                // 测试1: hello -> Hello World!
                display_hello();
            }
            else if (strcmp(cmd_msg.cmd, "wave") == 0) {
                // 测试2: wave -> 挥手图标
                display_wave();
            }
            else if (strcmp(cmd_msg.cmd, "smile") == 0) {
                // 测试3: smile -> 笑脸图标
                display_smile();
            }
            else if (strcmp(cmd_msg.cmd, "sad") == 0) {
                // 测试4: sad -> 哭脸图标
                display_sad();
            }
            else if (strcmp(cmd_msg.cmd, "clear") == 0) {
                // 测试5: clear -> 清空屏幕
                display_clear();
            }
            else {
                // 测试7: 未知命令
                display_invalid();
                vTaskDelay(pdMS_TO_TICKS(2000));  // 显示2秒后清屏
                display_clear();
            }
        }
    }
}

/**
 * @brief 主函数
 */
void app_main(void)
{
    esp_err_t ret;
    
    ESP_LOGI(TAG, "=== ESP32-S3 LCD Test Starting ===");
    
    // 初始化XL9555 I2C扩展芯片
    ESP_LOGI(TAG, "Initializing XL9555...");
    ret = xl9555_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "XL9555 init failed!");
        return;
    }
    
    // 初始化LCD
    ESP_LOGI(TAG, "Initializing LCD...");
    ret = lcd_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LCD init failed!");
        return;
    }
    
#if !CONFIG_USJ_ENABLE_USB_SERIAL_JTAG
    // 配置UART
    ESP_LOGI(TAG, "Configuring UART...");
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
#else
    ESP_LOGI(TAG, "Configuring USB Serial/JTAG...");
#endif
    
    // 创建命令队列
    cmd_queue = xQueueCreate(10, sizeof(cmd_msg_t));
    if (cmd_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create command queue!");
        return;
    }
    
    // 显示欢迎信息
    lcd_clear(BLUE);
    lcd_show_string(30, 120, "ESP32-S3", 32, WHITE, BLUE);
    lcd_show_string(40, 160, "LCD Test Ready", 24, YELLOW, BLUE);
    lcd_show_string(20, 200, "Send commands via", 16, WHITE, BLUE);
    lcd_show_string(70, 220, "UART", 16, WHITE, BLUE);
    
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    // 显示命令列表
    lcd_clear(BLACK);
    lcd_show_string(40, 20, "Available Commands:", 16, GREEN, BLACK);
    lcd_show_string(20, 50, "hello  - Hello World", 16, WHITE, BLACK);
    lcd_show_string(20, 75, "wave   - Wave icon", 16, WHITE, BLACK);
    lcd_show_string(20, 100, "smile  - Smile icon", 16, WHITE, BLACK);
    lcd_show_string(20, 125, "sad    - Sad icon", 16, WHITE, BLACK);
    lcd_show_string(20, 150, "clear  - Clear screen", 16, WHITE, BLACK);
    lcd_show_string(20, 200, "Waiting for command...", 16, YELLOW, BLACK);
    
    ESP_LOGI(TAG, "System ready. Waiting for commands...");
    ESP_LOGI(TAG, "Available commands: hello, wave, smile, sad, clear");
    
#if CONFIG_USJ_ENABLE_USB_SERIAL_JTAG
    // 创建USB Serial/JTAG命令输入任务
    xTaskCreate(usb_serial_jtag_task, "usb_serial_jtag_task", 4096, NULL, 10, NULL);
#else
    // 创建UART事件处理任务
    xTaskCreate(uart_event_task, "uart_event_task", 4096, NULL, 10, NULL);
#endif
    
    // 创建命令处理任务
    xTaskCreate(cmd_process_task, "cmd_process_task", 4096, NULL, 5, NULL);
}
