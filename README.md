# LeRobot ESP32 固件 for 微雪舵机驱动板

基于 [box2ai-robotics/lerobot-esp32](https://github.com/box2ai-robotics/lerobot-esp32) 项目，适配微雪 (Waveshare) ESP32 总线舵机驱动板。

## 硬件兼容性

- **主控**: ESP32-WROOM-32
- **舵机接口**: 半双工 UART (SN74LVC1G126/125 收发控制)
- **输入电压**: 6-12V (需与舵机电压匹配)
- **板载 OLED**: SSD1306 128x64
- **下载接口**: USB Type-C
- **通信方式**: UART / WiFi / Bluetooth / ESP-NOW
- **支持舵机**: Feetech SC/ST 系列、Hiwonder STS 系列总线舵机

## 功能特性

### 5 种工作模式

1. **Follower (从动端)** - 绿色
   - 接收 Leader 的位置数据并控制舵机
   - 反馈当前舵机状态

2. **Leader (主动端)** - 蓝色
   - 读取舵机位置并通过 ESP-NOW 发送给 Follower
   - 用于遥操作数据收集

3. **M-Leader (多从主动端)** - 深蓝色
   - 同时广播给多个 Follower
   - 用于控制多个机械臂

4. **Gateway (网关)** - 紫色
   - ESP-NOW ↔ USB 串口桥接
   - 用于连接 PC 进行数据收集和模型部署

5. **JoyCon (手柄控制)** - 灰色
   - 使用 Nintendo Joy-Con 手柄进行 IK 控制

### LED 指示灯

板载 WS2812 RGB LED (GPIO23):

| LED0 (模式) | 颜色 | LED1 (状态) | 颜色 | 模式 |
|------------|------|------------|------|------|
| Follower | 绿色 | 搜索/断开 | 橙色闪烁 | - |
| Leader | 蓝色 | 等待中 | 蓝色常亮 | - |
| M-Leader | 深蓝色 | 待确认 | 深蓝色闪烁 | - |
| Gateway | 紫色 | 已连接 | 绿色常亮 | - |
| JoyCon | 灰色 | 被接管 | 紫色常亮 | - |
| - | - | 过载保护 | 红色双闪 | - |

## 引脚定义

```cpp
// 舵机 UART
#define SERVO_TX_PIN    17    // UART2 TX
#define SERVO_RX_PIN    16    // UART2 RX
#define SERVO_TXEN_PIN  18    // TX 使能控制 (SN74LVC1G126)

// OLED 显示屏 (I2C)
#define OLED_SDA        21
#define OLED_SCL        22

// 用户按钮
#define BOOT_BUTTON     0     // GPIO0 (BOOT)

// RGB LED
#define LED_PIN         23    // WS2812

// UART 上位机通信 (UART0 - USB)
#define PC_UART_TX      1
#define PC_UART_RX      3
```

## 安装说明

### 方式一：PlatformIO (推荐)

PlatformIO 是更专业的嵌入式开发环境，支持 VS Code 集成，自动管理依赖库。

#### 1. 安装 PlatformIO

**方法 A：VS Code 扩展 (推荐)**
1. 安装 [Visual Studio Code](https://code.visualstudio.com/)
2. 在扩展市场搜索并安装 **PlatformIO IDE**
3. 重启 VS Code

**方法 B：命令行**
```bash
pip install platformio
```

#### 2. 克隆项目

```bash
git clone https://github.com/weihan-ott/lerobot-esp32-waveshare.git
cd lerobot-esp32-waveshare
```

#### 3. 编译固件

**使用 VS Code:**
1. 打开项目文件夹
2. 点击左下角 PlatformIO 图标
3. 选择 `esp32dev` → `Build`

**使用命令行:**
```bash
# 编译项目
pio run

# 编译并上传
pio run --target upload

# 上传后打开串口监视器
pio run --target upload && pio device monitor
```

#### 4. 上传固件到 ESP32

1. **连接硬件**: 用 USB Type-C 线连接驱动板到电脑
2. **选择端口** (自动或手动):
   ```bash
   # 查看可用端口
   pio device list
   
   # 指定端口上传 (如果需要)
   pio run --target upload --upload-port /dev/ttyUSB0  # Linux/Mac
   pio run --target upload --upload-port COM3          # Windows
   ```
3. **进入下载模式**:
   - 按住 **BOOT** 按钮
   - 点击 **RST** 按钮 (或插拔 USB 重新上电)
   - 松开 **BOOT** 按钮
4. **自动上传**: 运行上传命令后会自动烧录

#### 5. 常用 PlatformIO 命令

```bash
# 编译项目
pio run

# 上传固件
pio run --target upload

# 清理构建文件
pio run --target clean

# 打开串口监视器 (波特率 115200)
pio device monitor

# 上传并监控
pio run --target upload && pio device monitor

# 编译发布版本 (优化大小)
pio run -e esp32dev --target upload
```

#### 6. 项目配置

`platformio.ini` 已预配置好：
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

lib_deps = 
    adafruit/Adafruit SSD1306 @ ^2.5.9
    adafruit/Adafruit GFX Library @ ^1.11.9
    fastled/FastLED @ 3.7.8
    bblanchon/ArduinoJson @ ^6.21.4
```

---

### 方式二：Arduino IDE

如果你更喜欢 Arduino IDE，可以使用单文件版本。

#### 1. 安装依赖库

在 Arduino IDE 中安装以下库：
- `ESP32Servo` (ESP32 舵机控制)
- `Adafruit SSD1306` (OLED 显示屏)
- `Adafruit GFX Library` (图形库)
- `FastLED` (WS2812 LED 控制)
- `ArduinoJson` (JSON 序列化)

#### 2. 配置 Arduino IDE

1. 添加 ESP32 开发板管理器 URL:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```

2. 安装 ESP32 开发板 (版本 2.0.x 或更高)

3. 选择开发板：
   - 开发板: "ESP32 Dev Module"
   - Flash Mode: "QIO"
   - Flash Size: "4MB"
   - Partition Scheme: "Default 4MB with spiffs"

#### 3. 上传固件

1. 打开 `lerobot-esp32/lerobot-esp32.ino`
2. 用 USB Type-C 线连接驱动板到电脑
3. 选择正确的串口 (COMx 或 /dev/ttyUSBx)
4. 点击上传按钮

---

### 首次使用

#### 切换工作模式

上电后，**长按 BOOT 按钮 (GPIO0) 超过 2 秒** 切换工作模式。

#### 检查工作状态

- **OLED 显示屏**: 显示当前模式、舵机数量、连接状态
- **RGB LED**: 显示当前模式颜色
- **串口输出**: 连接串口监视器 (115200 波特率) 查看调试信息

## Web 控制界面

驱动板上电后会创建 WiFi 热点：
- **SSID**: `LeRobot-ESP32`
- **密码**: `12345678`

连接后访问 http://192.168.4.1 打开 Web 控制界面。

## PC 工具链

使用 Python 客户端与 Gateway 模式通信：

```python
from lerobot_client import LeRobotClient

client = LeRobotClient()
client.connect('/dev/ttyUSB0')  # 或 COM3 (Windows)

# 读取所有舵机位置
positions = client.get_positions()

# 控制单个舵机
client.set_position(servo_id=1, position=2048)

# 断开连接
client.disconnect()
```

## 项目结构

```
lerobot-esp32-waveshare/
├── src/
│   ├── main.cpp              # 主程序入口
│   ├── config.h              # 配置文件
│   ├── servo_driver.cpp      # 舵机驱动
│   ├── servo_driver.h
│   ├── wifi_manager.cpp      # WiFi 管理
│   ├── wifi_manager.h
│   ├── web_server.cpp        # Web 服务器
│   ├── web_server.h
│   ├── espnow_manager.cpp    # ESP-NOW 管理
│   ├── espnow_manager.h
│   ├── oled_display.cpp      # OLED 显示
│   ├── oled_display.h
│   ├── led_indicator.cpp     # LED 指示灯
│   ├── led_indicator.h
│   └── protocol.h            # 通信协议定义
├── data/
│   └── index.html            # Web 界面
├── python/
│   └── lerobot_client.py     # Python 客户端
├── platformio.ini            # PlatformIO 配置
└── README.md
```

## 通信协议

### ESP-NOW 数据包格式

```cpp
struct ServoDataPacket {
    uint8_t  packet_type;      // 0x01 = 位置数据
    uint8_t  servo_count;      // 舵机数量
    uint16_t servo_data[12];   // ID + 位置 (每个舵机 2 字节)
    uint32_t timestamp;        // 时间戳
    uint16_t crc;              // CRC 校验
} __attribute__((packed));
```

### 串口 JSON 协议 (Gateway 模式)

```json
// PC -> ESP32
{"cmd": "set_position", "id": 1, "pos": 2048}
{"cmd": "get_positions"}
{"cmd": "set_mode", "mode": "follower"}

// ESP32 -> PC
{"type": "positions", "data": [{"id": 1, "pos": 2048, "load": 128}], "ts": 1234567890}
{"type": "status", "mode": "leader", "connected": true}
```

## 与原版项目的区别

| 特性 | 原版 Box2Driver | 微雪适配版 |
|------|----------------|-----------|
| 硬件 | Box2Driver D1 | 微雪 ESP32 舵机驱动板 |
| 舵机接口 | 全双工 TTL | 半双工 UART (收发控制) |
| OLED | 可选外接 | 板载 128x64 |
| LED | 2x WS2812 | 1x WS2812 (GPIO23) |
| 供电 | 5V/12V | 6-12V |
| 按钮 | 2x 独立按钮 | 1x BOOT 按钮 |

## 参考资料

- [LeRobot 官方仓库](https://github.com/huggingface/lerobot)
- [box2ai/lerobot-esp32](https://github.com/box2ai-robotics/lerobot-esp32)
- [微雪 Wiki - Servo Driver with ESP32](https://www.waveshare.net/wiki/Servo_Driver_with_ESP32)
- [总线舵机驱动电路原理图](https://www.waveshare.net/w/upload/5/54/总线舵机驱动电路原理图.pdf)

## 许可证

Apache 2.0 License
