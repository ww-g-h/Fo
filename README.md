# ESP32-S3 LCD 测试项目

## 项目描述

这是一个ESP32-S3的LCD屏幕测试项目，使用MD0240 LCD模块（ST7789V驱动芯片，240x320分辨率）。

通过串口接收命令，在LCD屏幕上显示相应的内容（文字或图标）。

## 硬件连接

### GPIO 映射

| 功能 | ESP32-S3 GPIO | 说明 |
|-----|--------------|------|
| LCD SCK | GPIO12 | SPI时钟 |
| LCD MOSI | GPIO11 | SPI数据 |
| LCD CS | GPIO21 | SPI片选 |
| LCD DC | GPIO40 | 数据/命令选择 |
| LCD PWR | XL9555 P13 | 背光控制 |
| LCD RST | XL9555 P12 | 复位 |
| XL9555 SCL | GPIO42 | I2C时钟 |
| XL9555 SDA | GPIO41 | I2C数据 |
| UART TX | GPIO43 | 串口发送 |
| UART RX | GPIO44 | 串口接收 |

### XL9555 I2C扩展芯片

- I2C地址: 0x20
- 用于控制LCD的背光(PWR)和复位(RST)引脚

## 支持的测试命令

通过串口(115200波特率)发送以下命令：

| 测试编号 | 命令字符串 | 屏幕显示内容 | 说明 |
|---------|-----------|------------|------|
| 1 | `hello` | 显示 "Hello World!" | 验证基本显示功能 |
| 2 | `wave` | 显示挥手图标 + "Wave" | 测试图标显示 |
| 3 | `smile` | 显示笑脸图标 + "Smile" | 测试图像显示功能 |
| 4 | `sad` | 显示哭脸图标 + "Sad" | 测试不同图标切换 |
| 5 | `clear` | 清空屏幕 | 测试清屏功能 |
| 6 | 连续发送多个命令 | 按顺序显示 | 测试连续指令显示 |
| 7 | `unknown`等无效命令 | 显示 "Invalid command" | 测试错误处理 |

## 编译和烧录

### 1. 配置ESP-IDF环境

确保已安装ESP-IDF v5.5或更高版本。

```bash
# Windows PowerShell
. $env:IDF_PATH\export.ps1
```

### 2. 编译项目

```bash
cd E:\Fo\Fo
idf.py build
```

### 3. 烧录到ESP32-S3

```bash
idf.py -p COM端口 flash monitor
```

例如：
```bash
idf.py -p COM3 flash monitor
```

## 使用方法

1. 连接ESP32-S3开发板和LCD模块
2. 上电后，LCD屏幕会显示欢迎界面和可用命令列表
3. 通过串口工具（如Arduino IDE串口监视器、PuTTY等）发送命令
4. 观察LCD屏幕显示相应内容

### 串口配置

- 波特率: 115200
- 数据位: 8
- 停止位: 1
- 校验位: 无
- 流控: 无

### 测试示例

```
发送: hello
屏幕显示: Hello World!

发送: smile
屏幕显示: 笑脸图标

发送: wave
屏幕显示: 挥手图标

发送: sad
屏幕显示: 哭脸图标

发送: clear
屏幕显示: 清空屏幕（黑屏）

发送: test123
屏幕显示: Invalid command（红色背景，2秒后自动清屏）
```

## 项目结构

```
Fo/
├── CMakeLists.txt              # 项目主配置
├── README.md                   # 本文件
├── main/
│   ├── CMakeLists.txt
│   └── main.c                  # 主程序
└── components/
    ├── xl9555/                 # XL9555 I2C扩展芯片驱动
    │   ├── CMakeLists.txt
    │   ├── xl9555.h
    │   └── xl9555.c
    └── spilcd/                 # LCD驱动
        ├── CMakeLists.txt
        ├── spilcd.h
        ├── spilcd.c
        ├── font.h              # 字体定义
        ├── font.c
        ├── icons.h             # 图标定义
        └── icons.c
```

## 技术特点

1. **使用ESP-IDF v5.5新I2C驱动API**: 采用 `i2c_master` 驱动，替代旧的 `i2c_cmd` API
2. **FreeRTOS多任务**: UART接收和命令处理分别在不同任务中
3. **队列通信**: 使用FreeRTOS队列在任务间传递命令
4. **SPI高速通信**: 40MHz SPI时钟，快速刷新LCD
5. **模块化设计**: 驱动代码封装为独立组件

## 注意事项

1. 确保ESP-IDF版本为v5.5或更高，以使用新的I2C驱动API
2. LCD模块必须正确连接，否则可能无法显示
3. 命令不区分大小写（程序会自动转换为小写）
4. 命令最长31个字符
5. 图标数据为简化版本，实际项目可使用图片转换工具生成更精美的图标

## 故障排除

### LCD不显示

1. 检查SPI连接是否正确（SCK、MOSI、CS、DC引脚）
2. 检查I2C连接是否正确（SCL、SDA引脚）
3. 检查XL9555是否正常工作（I2C地址0x20）
4. 查看串口日志，确认初始化是否成功

### 串口无响应

1. 检查UART连接是否正确（TX、RX引脚）
2. 确认串口工具波特率设置为115200
3. 检查串口号是否正确

### 编译错误

1. 确认ESP-IDF版本：`idf.py --version`
2. 检查CMakeLists.txt配置
3. 清理重新编译：`idf.py fullclean && idf.py build`

## 许可证

MIT License
