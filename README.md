# LeRobot ESP32 主从机械臂同步固件

基于 ESP-NOW 无线通信的双机械臂主从同步控制系统，适配微雪 (Waveshare) ESP32 总线舵机驱动板。

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Build-blue.svg)](https://platformio.org/)
[![ESP32](https://img.shields.io/badge/ESP32-WROOM--32-green.svg)](https://www.espressif.com/)
[![License](https://img.shields.io/badge/License-Apache%202.0-orange.svg)](LICENSE)

## 📹 功能演示

- **主从同步**: 主臂运动时从臂实时跟随，延迟 < 50ms
- **无线配对**: 一键配对，无需配置网络
- **多模式支持**: 支持从臂、主臂、多从、网关、手柄等多种工作模式
- **OLED 显示**: 实时显示 MAC 地址、舵机数量、连接状态
- **LED 指示**: RGB LED 直观显示当前模式和连接状态

## 🎯 适用场景

- **AI 机器人学习**: 用于 LeRobot 项目的遥操作数据采集
- **机械臂教学**: 主臂演示动作，从臂同步学习
- **双臂协作**: 控制两个机械臂执行对称或镜像动作
- **远程操控**: 通过网关模式连接 PC 进行远程控制

## 📦 硬件要求

### 主控板
- **微雪 ESP32 总线舵机驱动板** (推荐)
- 或任何 ESP32 DevKit 配合半双工 UART 电路

### 舵机
- **STS3215** (推荐) - 12-bit 精度，0-4095 位置值
- 或其他 Feetech SC/ST 系列、Hiwonder STS 系列总线舵机
- **波特率**: 1Mbps (与舵机配置一致)

### 电源
- **输入电压**: 6-12V DC (根据舵机电压选择)
- **电流**: 建议 5A 以上 (6 个舵机同时运动)

### 配件
- USB Type-C 数据线 (供电和烧录)
- 杜邦线 (调试时使用)

## 🔌 引脚定义

```cpp
// ========== 舵机 UART (半双工) ==========
#define SERVO_RX_PIN    18    // UART2 RX
#define SERVO_TX_PIN    19    // UART2 TX
#define SERVO_TXEN_PIN  17    // TX 使能控制 (SN74LVC1G126/125)

// ========== OLED 显示屏 (I2C) ==========
#define OLED_SDA        21
#define OLED_SCL        22
#define OLED_ADDR       0x3C  // 128x32 分辨率

// ========== 用户按钮 ==========
#define BOOT_BUTTON     0     // GPIO0 (BOOT)，短按/长按/双击

// ========== RGB LED (WS2812) ==========
#define LED_PIN         23    // 板载 WS2812B
#define NUM_LEDS        1

// ========== 通信配置 ==========
#define SERVO_BAUD_RATE 1000000   // 舵机总线波特率 1Mbps
#define ESPNOW_CHANNEL  1         // WiFi 信道 1
#define SYNC_RATE_HZ    30        // 同步频率 30Hz
```

## 🚀 快速开始

### 方式一：PlatformIO (推荐)

#### 1. 安装 PlatformIO

**VS Code 扩展 (推荐)**
1. 安装 [Visual Studio Code](https://code.visualstudio.com/)
2. 扩展市场搜索并安装 **PlatformIO IDE**
3. 重启 VS Code

**命令行**
```bash
pip install platformio
```

#### 2. 克隆项目

```bash
git clone https://github.com/weihan-ott/lerobot-esp32-waveshare.git
cd lerobot-esp32-waveshare
```

#### 3. 编译并上传

```bash
# 编译项目
pio run

# 上传固件 (自动检测端口)
pio run --target upload

# 上传并打开串口监视器
pio run --target upload && pio device monitor
```

**进入下载模式** (如果自动下载失败):
1. 按住 **BOOT** 按钮
2. 按 **RST** 按钮 (或重新插拔 USB)
3. 松开 **BOOT** 按钮

### 方式二：Arduino IDE

1. 安装 ESP32 开发板支持 (版本 2.0.x+)
2. 安装依赖库:
   - `Adafruit SSD1306` @ 2.5.9+
   - `Adafruit GFX Library` @ 1.11.9+
   - `FastLED` @ 3.7.8
   - `ArduinoJson` @ 6.21.4+
3. 选择开发板: "ESP32 Dev Module"
4. 打开 `src/main.cpp`，复制内容到 Arduino IDE
5. 编译上传

## 🎮 使用说明

### 启动流程

上电后按以下流程初始化:

```
确认模式 → 选择模式 → 搜索舵机 → 配对 → 运行
```

1. **确认模式**: 显示上次保存的模式，短按确认，长按切换
2. **选择模式**: 短按切换模式，长按确认
3. **搜索舵机**: 自动扫描 ID 1-10 的舵机
4. **配对** (从臂模式): 扫描主臂并配对
5. **运行**: 进入正常工作状态

### 工作模式

| 模式 | 颜色 | 功能 | 配对前 | 配对后 |
|------|------|------|--------|--------|
| **Follower** | 🟢 绿色 | 从臂模式，接收主臂数据 | 蓝色闪烁 | 绿色常亮 |
| **Leader** | 🔵 蓝色 | 主臂模式，发送位置数据 | 蓝色常亮 | 绿色常亮 |
| **M-Leader** | 🔷 深蓝 | 多从臂广播 | 蓝色闪烁 | 蓝色常亮 |
| **Gateway** | 🟣 紫色 | 网关/桥接模式 | 紫色常亮 | - |
| **JoyCon** | ⚪ 灰色 | 手柄控制 (预留) | 灰色常亮 | - |

### 按键操作

| 操作 | 功能 |
|------|------|
| **短按** (< 1秒) | 确认选择 / 扫描 Leader |
| **长按** (> 2秒) | 切换模式 |
| **双击** | 重新配对 |

### 配对流程

**主臂 (Leader) 端:**
1. 上电后自动进入广播模式 (蓝色 LED)
2. 等待从臂连接，LED 变绿色表示配对成功

**从臂 (Follower) 端:**
1. 上电后进入扫描模式 (蓝色闪烁)
2. OLED 显示 "Found Leader: XX:XX:XX"
3. **短按 BOOT** 确认配对
4. LED 变绿色表示配对成功

配对成功后，主臂舵机运动会实时同步到从臂。

## 💡 LED 状态说明

| LED 状态 | 含义 |
|----------|------|
| 🟢 绿色常亮 | 已连接，正常工作 |
| 🔵 蓝色常亮 | 等待中 / 配对前 |
| 🔵 蓝色闪烁 | 扫描 / 搜索中 |
| 🟠 橙色闪烁 | 断开连接 |
| 🔴 红色快速闪烁 | 过载保护 |

## 🖥️ Web 控制界面

驱动板上电后创建 WiFi AP:
- **SSID**: `LeRobot-ESP32`
- **密码**: `12345678`

连接后访问 http://192.168.4.1 打开 Web 控制面板，可查看舵机状态、手动控制位置。

## 🐍 Python 客户端

```python
from python.lerobot_client import LeRobotClient

client = LeRobotClient()
client.connect('/dev/ttyUSB0')  # Linux/Mac
# client.connect('COM3')        # Windows

# 读取所有舵机位置
positions = client.get_positions()
print(positions)  # [{"id": 1, "pos": 2048, "load": 128}, ...]

# 控制单个舵机
client.set_position(servo_id=1, position=2048)

# 断开连接
client.disconnect()
```

## 📁 项目结构

```
lerobot-esp32-waveshare/
├── src/
│   ├── main.cpp              # 主程序入口
│   ├── config.h              # 硬件配置和常量
│   ├── servo_driver.cpp/h    # STS3215 舵机驱动 (半双工 UART)
│   ├── espnow_manager.cpp/h  # ESP-NOW 无线通信
│   ├── wifi_manager.cpp/h    # WiFi AP 和 Web 服务器
│   ├── web_server.cpp/h      # HTTP Web 接口
│   ├── oled_display.cpp/h    # SSD1306 128x32 显示
│   ├── led_indicator.cpp/h   # WS2812 RGB LED 控制
│   └── button_handler.cpp/h  # 按键处理 (短按/长按/双击)
├── python/
│   └── lerobot_client.py     # Python 串口通信客户端
├── platformio.ini            # PlatformIO 配置
└── README.md                 # 本文件
```

## 📡 通信协议

### ESP-NOW 数据包格式

```cpp
struct ServoDataPacket {
    uint8_t  packet_type;      // 0x01 = SYNC, 0x02 = STATUS
    uint8_t  servo_count;      // 舵机数量 (最多 12)
    uint16_t servo_data[12];   // 每个舵机: 高4位ID + 低12位位置
    uint32_t timestamp;        // 时间戳 (ms)
    uint16_t crc;              // CRC16 校验
} __attribute__((packed));

// servo_data 编码: (id << 12) | (position & 0x0FFF)
// 例如: ID=1, Pos=2048 → 0x1800
```

### 串口 JSON 协议 (Gateway 模式)

**PC → ESP32:**
```json
{"cmd": "set_position", "id": 1, "pos": 2048}
{"cmd": "get_positions"}
{"cmd": "set_mode", "mode": "follower"}
```

**ESP32 → PC:**
```json
{"type": "positions", "data": [{"id": 1, "pos": 2048}], "ts": 1234567890}
{"type": "status", "mode": "leader", "connected": true}
```

## 🔧 故障排除

### 1. OLED 不显示或显示乱码

**问题**: OLED 没有初始化或 I2C 地址错误

**解决**:
- 检查 `OLED_ADDR` 是否为 `0x3C` 或 `0x3D`
- 使用 I2C 扫描程序确认地址

### 2. 舵机不响应 / 搜索不到舵机

**问题**: 舵机通信失败

**解决**:
1. 确认舵机波特率设置为 **1Mbps** (可使用 Feetech 调试板修改)
2. 检查舵机电源供电是否充足 (建议 5A+)
3. 检查 TX_EN (GPIO17) 连接是否正常
4. 确认舵机 ID 设置正确 (1-10)

### 3. 主从臂无法配对

**问题**: ESP-NOW 通信失败

**解决**:
1. 确认两块板子的 WiFi 信道一致 (默认信道 1)
2. 确认距离不要太远 (< 10米)
3. 检查是否有 WiFi 干扰，尝试更换信道
4. 查看串口日志确认是否在扫描/广播

### 4. 配对成功但同步延迟高

**问题**: 通信或处理延迟

**解决**:
1. 减少舵机数量 (只连接需要同步的舵机)
2. 确保电源稳定，避免舵机卡顿
3. 检查是否有其他 WiFi 设备干扰 ESP-NOW

### 5. 从臂一直重启

**问题**: 看门狗超时

**解决**:
- 已修复，确保使用最新固件
- 如仍有问题，减少舵机读取频率或检查舵机通信

### 6. MAC 地址显示异常

**问题**: MAC 地址读取错误

**解决**:
- 确保使用 `WiFi.macAddress(ownMAC)` 而不是 `memcpy`
- 查看串口日志确认 MAC 格式正确

## 🔩 硬件接线图

### 微雪驱动板 (板载接线)

```
┌─────────────────────────────────────┐
│      微雪 ESP32 舵机驱动板           │
│                                     │
│  ┌──────┐  ┌──────────┐  ┌──────┐ │
│  │USB-C │  │  OLED    │  │ LED  │ │
│  │      │  │ 128x32   │  │ WS2812│ │
│  └──────┘  └──────────┘  └──────┘ │
│                                     │
│  ┌──────────────────────────────┐  │
│  │        舵机接口 (GH1.25)      │  │
│  │  GND  VCC(6-12V)  DATA       │  │
│  └──────────────────────────────┘  │
│                                     │
│  BOOT 按钮 ← GPIO0                  │
└─────────────────────────────────────┘
```

### 自定义接线 (使用 ESP32 DevKit)

```
ESP32 DevKit          舵机驱动电路
────────────          ──────────────
GPIO 18  ────────────►  RX (舵机)
GPIO 19  ────────────►  TX (舵机)
GPIO 17  ────────────►  TX_EN (收发控制)
GPIO 21  ────────────►  OLED SDA
GPIO 22  ────────────►  OLED SCL
GPIO 23  ────────────►  WS2812 LED
GPIO  0  ◄───────────   BOOT 按钮
3.3V     ────────────►  OLED VCC
5V       ────────────►  LED VCC
GND      ────────────►  共地
```

## 📋 版本历史

| 版本 | 日期 | 更新内容 |
|------|------|----------|
| v1.0 | 2024-04 | 初始版本，支持主从同步 |
| v1.1 | 2024-04 | 修复 CRC 验证、栈溢出、响应延迟问题 |

## 📚 参考资料

- [LeRobot 官方文档](https://github.com/huggingface/lerobot)
- [box2ai/lerobot-esp32](https://github.com/box2ai-robotics/lerobot-esp32) - 原版项目
- [微雪 Wiki - Servo Driver with ESP32](https://www.waveshare.net/wiki/Servo_Driver_with_ESP32)
- [Feetech STS3215 手册](http://www.feetech.cn/)
- [ESP-NOW 官方文档](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html)

## 🤝 贡献

欢迎提交 Issue 和 PR！

## 📄 许可证

Apache 2.0 License

Copyright 2024 LeRobot ESP32 Project
