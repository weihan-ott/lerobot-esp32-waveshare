# LeRobot ESP32 for Waveshare 舵机驱动板

本文件为 AI 编码助手提供项目背景、架构和开发指南。

## 项目概述

本项目是适配微雪 (Waveshare) ESP32 总线舵机驱动板的 LeRobot 固件，基于 [box2ai-robotics/lerobot-esp32](https://github.com/box2ai-robotics/lerobot-esp32) 项目移植。

用于机器人遥操作和数据收集，支持通过 ESP-NOW 无线协议同步多个机械臂的舵机位置。

### 硬件兼容性

- **主控**: ESP32-WROOM-32
- **舵机接口**: 半双工 UART (SN74LVC1G126/125 收发控制)
- **输入电压**: 6-12V (需与舵机电压匹配)
- **板载 OLED**: SSD1306 128x64 (I2C)
- **下载接口**: USB Type-C
- **通信方式**: UART / WiFi / ESP-NOW
- **支持舵机**: Feetech SC/ST 系列、Hiwonder STS 系列总线舵机

### 引脚定义

| 功能 | GPIO | 说明 |
|------|------|------|
| SERVO_TX | 17 | UART2 TX (舵机通信) |
| SERVO_RX | 16 | UART2 RX (舵机通信) |
| SERVO_TXEN | 18 | TX 使能控制 (半双工收发切换) |
| OLED_SDA | 21 | I2C 数据线 |
| OLED_SCL | 22 | I2C 时钟线 |
| BOOT_BUTTON | 0 | 用户按钮 (长按切换模式) |
| LED_PIN | 23 | WS2812 RGB LED |

## 技术栈

- **开发框架**: Arduino ESP32
- **构建系统**: PlatformIO
- **编程语言**: C/C++ (固件) / Python (客户端)
- **通信协议**: 
  - ESP-NOW (设备间无线通信)
  - STS 协议 (舵机控制)
  - JSON over Serial (PC 通信)

## 项目结构

```
lerobot-esp32-waveshare/
├── src/                          # 主源代码 (PlatformIO)
│   ├── main.cpp                  # 程序入口
│   ├── config.h                  # 配置参数
│   ├── servo_driver.cpp/h        # 舵机驱动 (STS 协议)
│   ├── espnow_manager.cpp/h      # ESP-NOW 管理
│   ├── wifi_manager.cpp/h        # WiFi AP 管理
│   ├── web_server.cpp/h          # Web 服务器
│   ├── oled_display.cpp/h        # OLED 显示
│   └── led_indicator.cpp/h       # LED 指示灯
├── lerobot-esp32/                # Arduino IDE 单文件版本
│   └── lerobot-esp32.ino         # 完整功能单文件版
├── python/                       # PC 端工具
│   └── lerobot_client.py         # Python 客户端库
├── platformio.ini                # PlatformIO 配置
└── README.md                     # 中文文档
```

## 工作模式

固件支持 5-6 种工作模式，通过长按 BOOT 按钮 (2秒) 循环切换。

| 模式 | 颜色 | LED0 | LED1 (状态) | 功能描述 |
|------|------|------|-------------|----------|
| Follower (从动端) | 绿色 | 绿色 | 橙色闪烁=搜索 | 接收 Leader 位置数据并控制舵机 |
| Leader (主动端) | 蓝色 | 蓝色 | 蓝色常亮=等待 | 读取位置并通过 ESP-NOW 发送 |
| M-Leader (多从主动端) | 深蓝色 | 深蓝色 | 深蓝色闪烁=待确认 | 广播给多个 Follower |
| Gateway (网关) | 紫色 | 紫色 | 绿色常亮=已连接 | ESP-NOW ↔ USB 串口桥接 |
| JoyCon (手柄控制) | 灰色 | 灰色 | 紫色常亮=被接管 | 使用 Nintendo Joy-Con 控制 |
| Serial Forward (串口转发) | 黄色 | - | - | USB ↔ 舵机直接透传 (仅 .ino 版本) |

## 构建和上传

### 使用 PlatformIO (推荐)

```bash
# 构建项目
pio run

# 上传固件到 ESP32
pio run --target upload

# 打开串口监视器
pio device monitor

# 清理构建文件
pio run --target clean
```

### 使用 Arduino IDE

1. 安装 ESP32 开发板支持 (版本 2.0.x 或更高)
2. 安装依赖库:
   - `Adafruit SSD1306` (^2.5.9)
   - `Adafruit GFX Library` (^1.11.9)
   - `FastLED` (^3.9.1)
   - `ArduinoJson` (^6.21.4)
3. 打开 `lerobot-esp32/lerobot-esp32.ino`
4. 选择开发板: "ESP32 Dev Module"
5. 上传

## 配置参数

主要配置在 `src/config.h` 中：

```cpp
// 舵机配置
#define MAX_SERVO_ID        20        // 最大支持舵机 ID
#define SERVO_BAUD_RATE     115200    // 舵机通信波特率

// ESP-NOW 配置
#define ESPNOW_CHANNEL      1
#define SYNC_RATE_HZ        30        // 同步频率 30Hz

// WiFi 配置
#define WIFI_SSID           "LeRobot-ESP32"
#define WIFI_PASSWORD       "12345678"

// 调试串口
#define DEBUG_BAUD_RATE     115200
```

## 通信协议

### ESP-NOW 数据包格式

```cpp
struct ServoDataPacket {
    uint8_t  packet_type;      // 0x01 = 位置数据, 0x02 = 状态报告
    uint8_t  servo_count;      // 舵机数量
    uint16_t servo_data[12];   // 高4位=ID, 低12位=位置
    uint32_t timestamp;        // 时间戳
    uint16_t crc;              // CRC 校验
} __attribute__((packed));
```

### 串口 JSON 协议 (Gateway 模式)

**PC -> ESP32:**
```json
{"cmd": "set_position", "id": 1, "pos": 2048, "speed": 500}
{"cmd": "get_positions"}
{"cmd": "set_mode", "mode": "follower"}
{"cmd": "scan"}
```

**ESP32 -> PC:**
```json
{"type": "positions", "data": [{"id": 1, "pos": 2048, "load": 128}], "ts": 1234567890}
{"type": "status", "mode": "leader", "connected": true}
```

## Python 客户端使用

```python
from lerobot_client import LeRobotClient

client = LeRobotClient()
client.connect('/dev/ttyUSB0')  # 或 COM3 (Windows)

# 扫描舵机
servos = client.scan_servos()

# 读取位置
positions = client.get_positions()

# 控制舵机
client.set_position(servo_id=1, position=2048, speed=500)

# 批量控制
client.set_positions({1: 2048, 2: 1024}, speed=500)

client.disconnect()
```

## 代码组织

### 模块职责

| 模块 | 职责 |
|------|------|
| `main.cpp` | 程序入口、模式切换调度、主循环 |
| `servo_driver` | STS 协议实现、舵机扫描、位置控制 |
| `espnow_manager` | ESP-NOW 初始化、数据收发、广播 |
| `wifi_manager` | WiFi AP 模式管理、热点创建 |
| `web_server` | HTTP 服务器、Web 控制界面 |
| `oled_display` | OLED 显示驱动、UI 画面 |
| `led_indicator` | WS2812 LED 控制、状态指示 |
| `config.h` | 所有配置参数集中定义 |

### 关键类说明

- **ServoDriver**: 舵机底层控制，使用 STS 协议通过半双工 UART 通信
- **ESPNowManager**: 管理 ESP-NOW 对等点、数据包收发、循环缓冲区
- **WebServerManager**: 提供 Web 控制接口和 REST API
- **OLEDDisplay**: 封装 SSD1306 显示，提供画面绘制方法
- **LEDIndicator**: 控制 RGB LED 颜色和动画效果

## 开发约定

### 代码风格

- 使用 Arduino C++ 风格
- 类名使用 PascalCase (如 `ServoDriver`)
- 函数和变量使用 camelCase (如 `setPosition`)
- 宏和常量使用 UPPER_SNAKE_CASE (如 `MAX_SERVO_ID`)
- 注释使用中文

### 调试输出

使用 `config.h` 中定义的宏：

```cpp
DEBUG_PRINT("message");
DEBUG_PRINTLN("message with newline");
DEBUG_PRINTF("formatted %s %d\n", str, num);
```

通过设置 `DEBUG_ENABLE` 控制是否输出调试信息。

### 错误处理

- 初始化失败时显示错误信息到 OLED 并停止运行
- 舵机通信超时处理
- ESP-NOW 发送失败重试

## 注意事项

1. **舵机电压**: 输入电压必须与舵机额定电压匹配 (6-12V)
2. **波特率**: 舵机默认 115200，但部分模式使用 1Mbps
3. **ID 范围**: 实际扫描 ID 1-10，但协议支持 ID 1-20
4. **模式切换**: 长按 BOOT 按钮 2 秒切换，短按无响应
5. **EEPROM**: 当前版本未实现模式持久化，每次重启恢复默认

## 依赖库版本

```ini
lib_deps = 
    adafruit/Adafruit SSD1306 @ ^2.5.9
    adafruit/Adafruit GFX Library @ ^1.11.9
    fastled/FastLED @ ^3.9.1
    bblanchon/ArduinoJson @ ^6.21.4
```

## 许可证

Apache 2.0 License
