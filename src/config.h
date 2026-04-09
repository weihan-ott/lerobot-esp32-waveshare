// config.h - 配置文件
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==================== 硬件引脚定义 ====================
// 舵机 UART (UART2 - 半双工)
#define SERVO_UART_NUM  2
#define SERVO_TX_PIN    17
#define SERVO_RX_PIN    16
#define SERVO_TXEN_PIN  18    // TX 使能控制 (SN74LVC1G126)

// OLED 显示屏 (I2C)
#define OLED_SDA        21
#define OLED_SCL        22
#define OLED_ADDRESS    0x3C
#define OLED_WIDTH      128
#define OLED_HEIGHT     64

// 用户按钮
#define BOOT_BUTTON     0     // GPIO0

// RGB LED (WS2812)
#define LED_PIN         23
#define NUM_LEDS        1

// PC 通信 UART (UART0 - USB)
#define PC_BAUD_RATE    1000000

// ==================== 舵机配置 ====================
// 支持的舵机类型
enum ServoType {
    SERVO_FEETECH_SC,   // Feetech SC 系列 (0-4095)
    SERVO_FEETECH_ST,   // Feetech ST 系列 (0-4095)
    SERVO_HIWONDER_STS  // Hiwonder STS 系列 (0-4095)
};

// 默认舵机配置
#define DEFAULT_SERVO_TYPE  SERVO_FEETECH_ST
#define MAX_SERVO_ID        20    // 最大支持舵机 ID
#define SERVO_BAUD_RATE     115200

// 舵机位置范围
#define SERVO_POS_MIN       0
#define SERVO_POS_MAX       4095
#define SERVO_POS_CENTER    2047

// 速度范围
#define SERVO_SPEED_MIN     0
#define SERVO_SPEED_MAX     3073

// ==================== ESP-NOW 配置 ====================
#define ESPNOW_CHANNEL      1
static const uint8_t ESPNOW_BROADCAST_ADDR[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
#define ESPNOW_BROADCAST    ESPNOW_BROADCAST_ADDR

// 数据包类型
#define PACKET_TYPE_SYNC    0x01  // 位置同步
#define PACKET_TYPE_STATUS  0x02  // 状态报告
#define PACKET_TYPE_CMD     0x03  // 控制命令

// 同步频率
#define SYNC_RATE_HZ        30    // 30Hz 同步频率
#define SYNC_INTERVAL_MS    (1000 / SYNC_RATE_HZ)

// ==================== WiFi 配置 ====================
#define WIFI_SSID           "LeRobot-ESP32"
#define WIFI_PASSWORD       "12345678"
#define WIFI_CHANNEL        1
#define WIFI_MAX_CLIENTS    4

// Web 服务器
#define WEB_SERVER_PORT     80

// ==================== 工作模式 ====================
enum DeviceMode {
    MODE_FOLLOWER = 0,  // 从动端 (绿色)
    MODE_LEADER = 1,    // 主动端 (蓝色)
    MODE_M_LEADER = 2,  // 多从主动端 (深蓝色)
    MODE_GATEWAY = 3,   // 网关 (紫色)
    MODE_JOYCON = 4     // 手柄控制 (灰色)
};

#define NUM_MODES 5

// 模式颜色 (RGB)
const uint32_t MODE_COLORS[NUM_MODES] = {
    0x00FF00,   // Follower - 绿色
    0x0000FF,   // Leader - 蓝色
    0x000080,   // M-Leader - 深蓝色
    0x800080,   // Gateway - 紫色
    0x808080    // JoyCon - 灰色
};

// ==================== 状态颜色 ====================
#define STATUS_DISCONNECTED  0xFF8C00  // 橙色 (断开)
#define STATUS_WAITING       0x0000FF  // 蓝色 (等待)
#define STATUS_PENDING       0x000080  // 深蓝色 (待确认)
#define STATUS_CONNECTED     0x00FF00  // 绿色 (已连接)
#define STATUS_TAKEOVER      0x800080  // 紫色 (被接管)
#define STATUS_OVERLOAD      0xFF0000  // 红色 (过载)

// ==================== 按键配置 ====================
#define BUTTON_DEBOUNCE_MS   50
#define BUTTON_LONG_PRESS_MS 2000  // 长按 2 秒切换模式
#define BUTTON_SCAN_INTERVAL 100   // 扫描间隔

// ==================== EEPROM 配置 ====================
#define EEPROM_SIZE          512
#define EEPROM_MODE_ADDR     0     // 保存当前模式
#define EEPROM_PEER_ADDR     10    // 保存对端 MAC 地址

// ==================== 串口协议配置 ====================
#define SERIAL_BUFFER_SIZE   256
#define JSON_DOC_SIZE        512

// ==================== 调试配置 ====================
#define DEBUG_ENABLE         true
#define DEBUG_BAUD_RATE      115200

#if DEBUG_ENABLE
    #define DEBUG_PRINT(x) Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
    #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(...)
#endif

#endif // CONFIG_H
