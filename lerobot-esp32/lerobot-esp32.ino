// LeRobot ESP32 for Waveshare Servo Driver Board
// 这是 Arduino IDE 单文件版本
// 完整多文件版本请使用 PlatformIO

// ==================== 配置 ====================
// 硬件引脚定义
#define SERVO_TX_PIN    17    // UART2 TX
#define SERVO_RX_PIN    16    // UART2 RX
#define SERVO_TXEN_PIN  18    // TX 使能控制
#define OLED_SDA        21
#define OLED_SCL        22
#define BOOT_BUTTON     0
#define LED_PIN         23

// 串口波特率 - 舵机协议使用 1Mbps
#define SERVO_BAUD_RATE 1000000
#define DEBUG_BAUD_RATE 115200

// ==================== 库包含 ====================
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FastLED.h>
#include <esp_now.h>
#include <WiFi.h>

// ==================== 常量定义 ====================
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C
#define NUM_LEDS      1
#define MIN_SERVO_ID  1
#define MAX_SERVO_ID  10  // 只扫描和控制 ID 1-10

// STS 协议命令
#define STS_CMD_PING  0x01
#define STS_CMD_READ  0x02
#define STS_CMD_WRITE 0x03

// STS 寄存器
#define STS_REG_ID          0x05
#define STS_REG_TORQUE      0x28
#define STS_REG_TARGET_POS  0x2A
#define STS_REG_SPEED       0x2E
#define STS_REG_CURRENT_POS 0x38

// ESP-NOW 数据包
struct ServoDataPacket {
    uint8_t  packet_type;
    uint8_t  servo_count;
    uint16_t servo_data[12];
    uint32_t timestamp;
    uint16_t crc;
} __attribute__((packed));

// 工作模式 - 6种模式循环切换
enum DeviceMode {
    MODE_FOLLOWER = 0,       // 从动端
    MODE_LEADER = 1,         // 主动端
    MODE_M_LEADER = 2,       // 多从主动端
    MODE_GATEWAY = 3,        // 网关
    MODE_JOYCON = 4,         // 手柄控制
    MODE_SERIAL_FORWARD = 5  // 串口转发模式
};

#define NUM_MODES 6  // 总模式数

// ==================== 全局变量 ====================
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
CRGB leds[NUM_LEDS];
DeviceMode currentMode = MODE_FOLLOWER;

HardwareSerial& servoSerial = Serial2;
bool servoOnline[MAX_SERVO_ID + 1] = {false};
uint16_t servoPositions[MAX_SERVO_ID + 1];
uint16_t servoTargets[MAX_SERVO_ID + 1];
bool modeInitialized = false;  // 标记当前模式是否已初始化

// 模式颜色定义
const CRGB MODE_COLORS[NUM_MODES] = {
    CRGB::Green,      // Follower - 绿色
    CRGB::Blue,       // Leader - 蓝色
    CRGB(0, 0, 128),  // M-Leader - 深蓝色
    CRGB::Purple,     // Gateway - 紫色
    CRGB::Gray,       // JoyCon - 灰色
    CRGB::Yellow      // Serial Forward - 黄色
};

// 模式名称
const char* MODE_NAMES[NUM_MODES] = {
    "Follower",
    "Leader", 
    "M-Leader",
    "Gateway",
    "JoyCon",
    "SerialFwd"
};

// ESP-NOW 接收回调
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    if (len == sizeof(ServoDataPacket)) {
        ServoDataPacket packet;
        memcpy(&packet, incomingData, sizeof(packet));
        
        // Follower 模式：应用位置（只处理 ID 1-10）
        if (currentMode == MODE_FOLLOWER) {
            for (int i = 0; i < packet.servo_count && i < 12; i++) {
                uint8_t id = (packet.servo_data[i] >> 12) & 0x0F;
                uint16_t pos = packet.servo_data[i] & 0x0FFF;
                if (id >= MIN_SERVO_ID && id <= MAX_SERVO_ID) {
                    servoTargets[id] = pos;
                    setServoPosition(id, pos);
                }
            }
            // LED 显示连接状态
            leds[0] = CRGB::Green;
            FastLED.show();
        }
    }
}

// ==================== 舵机控制函数 ====================
void txEnable() {
    digitalWrite(SERVO_TXEN_PIN, HIGH);
    delayMicroseconds(10);
}

void rxEnable() {
    digitalWrite(SERVO_TXEN_PIN, LOW);
    delayMicroseconds(10);
}

void sendServoCommand(uint8_t id, uint8_t instruction, uint8_t* params, uint8_t paramLen) {
    uint8_t packet[16];
    uint8_t idx = 0;
    
    // 包头
    packet[idx++] = 0xFF;
    packet[idx++] = 0xFF;
    
    // ID
    packet[idx++] = id;
    
    // 长度
    packet[idx++] = paramLen + 2;
    
    // 指令
    packet[idx++] = instruction;
    
    // 参数
    for (uint8_t i = 0; i < paramLen; i++) {
        packet[idx++] = params[i];
    }
    
    // 校验和
    uint8_t checksum = 0;
    for (uint8_t i = 2; i < idx; i++) {
        checksum += packet[i];
    }
    packet[idx++] = ~checksum;
    
    // 发送
    txEnable();
    servoSerial.write(packet, idx);
    servoSerial.flush();
    delayMicroseconds(200);
    rxEnable();
}

bool pingServo(uint8_t id) {
    sendServoCommand(id, STS_CMD_PING, NULL, 0);
    
    // 等待响应
    uint32_t start = millis();
    while (millis() - start < 50) {  // 50ms 超时
        if (servoSerial.available() >= 6) {
            uint8_t buffer[16];
            uint8_t idx = 0;
            while (servoSerial.available() && idx < 16) {
                buffer[idx++] = servoSerial.read();
            }
            
            // 简单检查响应
            if (idx >= 6 && buffer[0] == 0xFF && buffer[1] == 0xFF && buffer[2] == id) {
                return true;
            }
        }
    }
    return false;
}

void setServoPosition(uint8_t id, uint16_t position) {
    if (id < MIN_SERVO_ID || id > MAX_SERVO_ID) return;
    
    uint8_t low = position & 0xFF;
    uint8_t high = (position >> 8) & 0xFF;
    uint8_t params[5] = {STS_REG_TARGET_POS, low, high, 0, 0};
    sendServoCommand(id, STS_CMD_WRITE, params, 5);
}

uint16_t getServoPosition(uint8_t id) {
    if (id < MIN_SERVO_ID || id > MAX_SERVO_ID) return 0;
    
    uint8_t params[2] = {STS_REG_CURRENT_POS, 2};
    sendServoCommand(id, STS_CMD_READ, params, 2);
    
    uint32_t start = millis();
    while (millis() - start < 50) {
        if (servoSerial.available() >= 8) {
            uint8_t buffer[16];
            uint8_t idx = 0;
            while (servoSerial.available() && idx < 16) {
                buffer[idx++] = servoSerial.read();
            }
            
            // 解析位置数据
            if (idx >= 8 && buffer[0] == 0xFF && buffer[1] == 0xFF) {
                uint8_t low = buffer[5];
                uint8_t high = buffer[6];
                return (high << 8) | low;
            }
        }
    }
    return 0;
}

int scanServos() {
    int count = 0;
    
    // OLED 显示扫描中
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.printf("Mode: %s", MODE_NAMES[currentMode]);
    display.setCursor(0, 16);
    display.println("Scanning ID 1-10...");
    display.display();
    
    for (uint8_t id = MIN_SERVO_ID; id <= MAX_SERVO_ID; id++) {
        if (pingServo(id)) {
            servoOnline[id] = true;
            servoPositions[id] = getServoPosition(id);
            servoTargets[id] = servoPositions[id];
            count++;
            
            // 显示发现的舵机
            display.setCursor(0, 32 + (count - 1) * 8);
            display.printf("ID%d:%d", id, servoPositions[id]);
            display.display();
        } else {
            servoOnline[id] = false;
        }
        delay(5);
    }
    
    Serial.printf("Found %d servos (ID 1-10)\n", count);
    
    // 显示扫描结果
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.printf("Mode: %s", MODE_NAMES[currentMode]);
    display.setCursor(0, 16);
    display.printf("Found %d/10 servos", count);
    
    // 列出在线舵机
    int y = 28;
    for (int i = MIN_SERVO_ID; i <= MAX_SERVO_ID && y < 64; i++) {
        if (servoOnline[i]) {
            display.setCursor(0, y);
            display.printf("ID%d OK", i);
            y += 10;
        }
    }
    display.display();
    delay(1000);  // 显示1秒
    
    return count;
}

// ==================== 串口转发功能 ====================
void handleSerialForwarding() {
    // USB -> 舵机
    while (Serial.available()) {
        uint8_t data = Serial.read();
        txEnable();
        servoSerial.write(data);
        servoSerial.flush();
        rxEnable();
    }
    
    // 舵机 -> USB
    while (servoSerial.available()) {
        uint8_t data = servoSerial.read();
        Serial.write(data);
    }
}

// ==================== 显示函数 ====================
void showStartup() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(10, 10);
    display.println("LeRobot");
    display.setTextSize(1);
    display.setCursor(20, 35);
    display.println("ESP32 Waveshare");
    display.setCursor(25, 50);
    display.println("Initializing...");
    display.display();
}

void showMode(DeviceMode mode) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(10, 10);
    display.println("Mode");
    display.drawLine(0, 30, 128, 30, SSD1306_WHITE);
    display.setCursor(10, 40);
    display.println(MODE_NAMES[mode]);
    display.display();
}

void showStatus(const char* mac) {
    display.clearDisplay();
    
    // 第1行: 模式（字号2，醒目）
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.print(MODE_NAMES[currentMode]);
    
    display.setTextSize(1);  // 恢复字号1
    
    // 串口转发模式特殊显示
    if (currentMode == MODE_SERIAL_FORWARD) {
        display.setCursor(0, 18);
        display.print("MAC:");
        display.print(mac);
        display.setCursor(0, 28);
        display.print("Baud:1M ID:1-10");
        display.setCursor(0, 40);
        display.print("Bridge Active");
        display.display();
        return;
    }
    
    // 第2行: MAC地址
    display.setCursor(0, 18);
    display.print("MAC:");
    display.print(mac);
    
    // 第3行: 在线舵机数
    int onlineCount = 0;
    for (int i = MIN_SERVO_ID; i <= MAX_SERVO_ID; i++) {
        if (servoOnline[i]) onlineCount++;
    }
    display.setCursor(0, 28);
    display.print("Servos:");
    display.print(onlineCount);
    display.print("/10");
    
    // 第4行: 连接状态
    display.setCursor(0, 40);
    if (currentMode == MODE_LEADER || currentMode == MODE_M_LEADER) {
        display.print("Status: Sending");
    } else if (currentMode == MODE_FOLLOWER) {
        display.print("Status: Receiving");
    } else if (currentMode == MODE_GATEWAY) {
        display.print("Status: Bridge");
    } else if (currentMode == MODE_JOYCON) {
        display.print("Status: JoyCon");
    }
    
    display.display();
}

// ==================== 模式初始化 ====================
void initializeMode() {
    Serial.printf("Initializing mode: %s\n", MODE_NAMES[currentMode]);
    
    // 显示模式
    showMode(currentMode);
    delay(500);
    
    // 串口转发模式不需要扫描舵机
    if (currentMode == MODE_SERIAL_FORWARD) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println("Serial Forward");
        display.setCursor(0, 16);
        display.println("Direct Pass-through");
        display.setCursor(0, 32);
        display.println("Baud: 1000000");
        display.display();
        
        Serial.println("Serial Forward mode: USB <-> Servo direct");
        modeInitialized = true;
        return;
    }
    
    // 其他模式扫描舵机 ID 1-10
    scanServos();
    
    modeInitialized = true;
}

// ==================== 主程序 ====================
void setup() {
    // 初始化调试串口
    Serial.begin(DEBUG_BAUD_RATE);
    Serial.println("\n================================");
    Serial.println("LeRobot ESP32 Starting...");
    Serial.println("Servo Range: ID 1-10");
    Serial.println("Baud Rate: 1000000");
    Serial.println("Long press BOOT (2s) to switch mode");
    Serial.println("================================");
    
    // 初始化按钮
    pinMode(BOOT_BUTTON, INPUT_PULLUP);
    
    // 初始化 LED
    FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(50);
    leds[0] = CRGB::White;
    FastLED.show();
    
    // 初始化 I2C 和 OLED
    Wire.begin(OLED_SDA, OLED_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println("OLED init failed!");
    }
    
    showStartup();
    delay(1000);
    
    // 初始化舵机串口 - 1Mbps
    pinMode(SERVO_TXEN_PIN, OUTPUT);
    rxEnable();
    servoSerial.begin(SERVO_BAUD_RATE, SERIAL_8N1, SERVO_RX_PIN, SERVO_TX_PIN);
    delay(100);
    
    Serial.printf("Servo UART initialized at %d baud\n", SERVO_BAUD_RATE);
    
    // 初始化 WiFi 和 ESP-NOW
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    char macStr[18];
    uint8_t mac[6];
    WiFi.macAddress(mac);
    sprintf(macStr, "%02X:%02X:%02X", mac[3], mac[4], mac[5]);
    
    if (esp_now_init() == ESP_OK) {
        Serial.println("ESP-NOW initialized");
        esp_now_register_recv_cb(OnDataRecv);
        
        // 添加广播对等点
        esp_now_peer_info_t peerInfo = {};
        uint8_t broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        memcpy(peerInfo.peer_addr, broadcast, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;
        esp_now_add_peer(&peerInfo);
    } else {
        Serial.println("ESP-NOW init failed");
    }
    
    // 设置 AP 模式
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("LeRobot-ESP32", "12345678");
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    
    // 设置 LED 为模式颜色
    leds[0] = MODE_COLORS[currentMode];
    FastLED.show();
    
    // 初始化当前模式
    initializeMode();
    
    Serial.print("Current mode: ");
    Serial.println(MODE_NAMES[currentMode]);
    Serial.println("Setup complete!");
}

void loop() {
    // 检查按钮（长按2秒切换模式）
    static bool buttonWasPressed = false;
    static unsigned long buttonPressTime = 0;
    static bool modeSwitchTriggered = false;
    
    bool buttonPressed = (digitalRead(BOOT_BUTTON) == LOW);
    
    if (buttonPressed && !buttonWasPressed) {
        buttonWasPressed = true;
        buttonPressTime = millis();
        modeSwitchTriggered = false;
    } else if (buttonPressed && buttonWasPressed && !modeSwitchTriggered) {
        // 按钮持续按住，检查是否达到2秒
        if (millis() - buttonPressTime >= 2000) {
            // 长按2秒，切换模式
            modeSwitchTriggered = true;
            
            // 切换到下一个模式（循环）
            currentMode = (DeviceMode)((currentMode + 1) % NUM_MODES);
            modeInitialized = false;  // 重置初始化标记
            
            Serial.printf("Mode switched to: %s (%d)\n", MODE_NAMES[currentMode], currentMode);
            
            // 更新 LED
            leds[0] = MODE_COLORS[currentMode];
            FastLED.show();
            
            // 初始化新模式
            initializeMode();
        }
    } else if (!buttonPressed && buttonWasPressed) {
        // 按钮释放
        buttonWasPressed = false;
        modeSwitchTriggered = false;
    }
    
    // 根据当前模式执行不同功能
    switch (currentMode) {
        case MODE_FOLLOWER:
            // Follower 模式：接收 ESP-NOW 数据并控制 ID 1-10
            // 接收处理在 OnDataRecv 回调中完成
            // LED 闪烁表示接收到数据
            break;
            
        case MODE_LEADER:
            // Leader 模式：读取 ID 1-10 位置并通过 ESP-NOW 发送
            {
                static unsigned long lastSend = 0;
                if (millis() - lastSend > 33) {  // 30Hz
                    lastSend = millis();
                    
                    ServoDataPacket packet;
                    packet.packet_type = 0x01;
                    packet.servo_count = 0;
                    packet.timestamp = millis();
                    
                    // 只读取 ID 1-10
                    for (uint8_t id = MIN_SERVO_ID; id <= MAX_SERVO_ID && packet.servo_count < 12; id++) {
                        if (servoOnline[id]) {
                            servoPositions[id] = getServoPosition(id);
                            packet.servo_data[packet.servo_count] = (id << 12) | (servoPositions[id] & 0x0FFF);
                            packet.servo_count++;
                        }
                    }
                    
                    packet.crc = 0xFFFF;
                    
                    if (packet.servo_count > 0) {
                        uint8_t broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
                        esp_now_send(broadcast, (uint8_t*)&packet, sizeof(packet));
                        
                        // 发送时 LED 亮绿色
                        leds[0] = CRGB::Green;
                        FastLED.show();
                        delayMicroseconds(5000);
                        leds[0] = MODE_COLORS[currentMode];
                        FastLED.show();
                    }
                }
            }
            break;
            
        case MODE_M_LEADER:
            // M-Leader 模式：广播 ID 1-10 位置
            {
                static unsigned long lastSend = 0;
                if (millis() - lastSend > 33) {
                    lastSend = millis();
                    
                    ServoDataPacket packet;
                    packet.packet_type = 0x01;
                    packet.servo_count = 0;
                    packet.timestamp = millis();
                    
                    for (uint8_t id = MIN_SERVO_ID; id <= MAX_SERVO_ID && packet.servo_count < 12; id++) {
                        if (servoOnline[id]) {
                            servoPositions[id] = getServoPosition(id);
                            packet.servo_data[packet.servo_count] = (id << 12) | (servoPositions[id] & 0x0FFF);
                            packet.servo_count++;
                        }
                    }
                    
                    packet.crc = 0xFFFF;
                    
                    if (packet.servo_count > 0) {
                        uint8_t broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
                        esp_now_send(broadcast, (uint8_t*)&packet, sizeof(packet));
                        
                        leds[0] = CRGB::Green;
                        FastLED.show();
                        delayMicroseconds(5000);
                        leds[0] = MODE_COLORS[currentMode];
                        FastLED.show();
                    }
                }
            }
            break;
            
        case MODE_GATEWAY:
            // Gateway 模式：ESP-NOW 与 USB 串口桥接
            // TODO: 实现完整的数据桥接功能
            break;
            
        case MODE_JOYCON:
            // JoyCon 模式：手柄控制 ID 1-10
            // TODO: 实现 JoyCon 蓝牙连接
            break;
            
        case MODE_SERIAL_FORWARD:
            // 串口转发模式：USB <-> 舵机直接通信（1Mbps）
            handleSerialForwarding();
            
            // 每 500ms 更新一次显示，避免影响转发性能
            static unsigned long lastSfDisplay = 0;
            if (millis() - lastSfDisplay > 500) {
                lastSfDisplay = millis();
                
                display.clearDisplay();
                display.setTextSize(1);
                display.setCursor(0, 0);
                display.println("Serial Forward Mode");
                display.setCursor(0, 12);
                display.println("Baud: 1000000");
                display.setCursor(0, 24);
                display.println("ID Range: 1-10");
                display.setCursor(0, 36);
                display.println("Direct Pass-through");
                display.setCursor(0, 48);
                display.println("Ready for PC Tool");
                display.display();
            }
            break;
    }
    
    // 更新显示（非串口转发模式下）
    if (currentMode != MODE_SERIAL_FORWARD) {
        static unsigned long lastDisplay = 0;
        if (millis() - lastDisplay > 500) {
            lastDisplay = millis();
            
            char macStr[18];
            uint8_t mac[6];
            WiFi.macAddress(mac);
            sprintf(macStr, "%02X:%02X:%02X", mac[3], mac[4], mac[5]);
            
            showStatus(macStr);
        }
    }
    
    delay(1);
}