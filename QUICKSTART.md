# ESP32-S3 LCD测试 - 快速开始

## 1. 准备工作

### 硬件需求
- ESP32-S3 开发板
- MD0240 LCD模块（带XL9555 I2C扩展芯片）
- USB数据线

### 软件需求
- ESP-IDF v5.5 或更高版本
- Python 3.7+ （用于测试脚本）
- 串口终端工具（可选）

## 2. 连接硬件

按照以下GPIO映射连接LCD模块：

```
LCD SCK  → GPIO12
LCD MOSI → GPIO11
LCD CS   → GPIO21
LCD DC   → GPIO40

XL9555 SCL → GPIO42
XL9555 SDA → GPIO41

UART TX → GPIO43 (默认)
UART RX → GPIO44 (默认)
```

## 3. 编译和烧录

### 方法1: 使用构建脚本（推荐）

```powershell
# 在PowerShell中执行
cd E:\Fo\Fo

# 编译、烧录并打开监视器（一步完成）
.\build.ps1 -Action all -Port COM3

# 或者分步执行：
.\build.ps1 -Action build         # 仅编译
.\build.ps1 -Action flash -Port COM3   # 烧录
.\build.ps1 -Action monitor -Port COM3 # 监视器
```

### 方法2: 使用ESP-IDF命令

```bash
# 配置ESP-IDF环境
. $env:IDF_PATH\export.ps1

# 编译
idf.py build

# 烧录
idf.py -p COM3 flash

# 打开监视器
idf.py -p COM3 monitor
```

## 4. 运行测试

### 方法1: 使用Python测试脚本（推荐）

```bash
# 安装pyserial（首次运行）
pip install pyserial

# 运行自动测试
python test_lcd.py -p COM3

# 自定义波特率
python test_lcd.py -p COM5 -b 115200
```

### 方法2: 使用PowerShell测试脚本

```powershell
.\test_lcd.ps1 -Port COM3
```

### 方法3: 手动测试

使用任何串口终端工具（115200波特率）发送以下命令：

```
hello   → 显示 "Hello World!"
wave    → 显示挥手图标
smile   → 显示笑脸图标
sad     → 显示哭脸图标
clear   → 清空屏幕
```

## 5. 验证测试结果

### 测试1: hello
- **预期**: 黑色背景，白色文字 "Hello World!"
- **位置**: 屏幕中央

### 测试2: wave
- **预期**: 白色背景，挥手图标 + "Wave" 文字
- **位置**: 图标在上方，文字在下方

### 测试3: smile
- **预期**: 白色背景，黄色笑脸图标 + "Smile" 文字
- **位置**: 图标在上方，文字在下方

### 测试4: sad
- **预期**: 白色背景，黄色哭脸图标（带蓝色泪滴）+ "Sad" 文字
- **位置**: 图标在上方，文字在下方

### 测试5: clear
- **预期**: 纯黑色屏幕
- **无内容显示**

### 测试6: 连续命令
依次发送 `hello`、`smile`、`wave`
- **预期**: 屏幕内容按顺序切换，无延迟或错误

### 测试7: 无效命令
发送 `unknown` 或其他未定义命令
- **预期**: 红色背景，白色文字 "Invalid command"，2秒后自动清屏

## 6. 查看日志

在串口监视器中可以看到详细的运行日志：

```
I (123) MAIN: === ESP32-S3 LCD Test Starting ===
I (234) MAIN: Initializing XL9555...
I (345) XL9555: XL9555 initialized successfully
I (456) MAIN: Initializing LCD...
I (567) LCD: Initializing LCD...
I (678) LCD: LCD initialized successfully
I (789) MAIN: System ready. Waiting for commands...
I (890) MAIN: Received command: hello
I (901) MAIN: Processing command: hello
I (912) MAIN: Displayed: Hello World!
```

## 7. 故障排除

### LCD不亮或显示异常
1. 检查I2C连接（GPIO42/41）
2. 检查背光控制（XL9555 P13）
3. 查看日志中的错误信息

### 串口无响应
1. 确认COM端口号正确
2. 检查波特率设置（115200）
3. 确认UART引脚连接正确

### 编译错误
1. 检查ESP-IDF版本：`idf.py --version`
2. 清理重新编译：`idf.py fullclean && idf.py build`
3. 确认所有依赖组件都已正确配置

## 8. 下一步

- 修改图标数据，使用更精美的图标
- 添加更多命令和显示效果
- 实现触摸屏功能（如果硬件支持）
- 添加动画效果

## 9. 技术支持

如有问题，请查看：
1. 项目日志输出
2. README.md 中的详细文档
3. ESP-IDF官方文档

---
**祝测试顺利！** 🎉
