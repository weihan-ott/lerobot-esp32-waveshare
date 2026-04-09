// main.cpp - LeRobot ESP32 主程序
// 适配微雪 (Waveshare) ESP32 总线舵机驱动板

#include "config.h"
#include "servo_driver.h"
#include "espnow_manager.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "oled_display.h"
#include "led_indicator.h"

// 全局对象
ServoDriver servoDriver;
ESPNowManager espNowManager;
WiFiManager wifiManager;
WebServerManager webServer;
OLEDDisplay oledDisplay;
LEDIndicator ledIndicator;

// 当前工作模式
volatile DeviceMode currentMode = MODE_FOLLOWER;
volatile DeviceMode lastMode = MODE_FOLLOWER;

// 按钮状态
unsigned long buttonPressTime = 0;
bool buttonPressed = false;
bool buttonShortPress = false;  // 短按标志

// 同步数据
ServoDataPacket syncPacket;
uint32_t lastSyncTime = 0;

// ESP-NOW 配对状态
bool inPairingMode = false;
int selectedPeerIndex = 0;
uint8_t pairedPeerMAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
bool pairingCompleted = false;

// 函数声明
void setup();
void loop();
void handleButton();
void modeFollower();
void modeLeader();
void modeMLeader();
void modeGateway();
void modeJoyCon();
void switchMode();
void loadSavedMode();
void saveCurrentMode();
void enterPairingMode();
void selectPeerFromList();
void enterFollowerMode();

void setup() {
    // 初始化调试串口
    Serial.begin(DEBUG_BAUD_RATE);
    DEBUG_PRINTLN("\n================================");
    DEBUG_PRINTLN("LeRobot ESP32 for Waveshare");
    DEBUG_PRINTLN("Initializing...");
    DEBUG_PRINTLN("================================\n");
    
    // 初始化按钮
    pinMode(BOOT_BUTTON, INPUT_PULLUP);
    
    // 初始化 LED
    ledIndicator.begin();
    ledIndicator.setColor(0xFFFFFF);  // 白色启动
    
    // 初始化 OLED
    if (!oledDisplay.begin()) {
        DEBUG_PRINTLN("OLED init failed!");
    }
    oledDisplay.showStartup();
    
    // 初始化舵机驱动
    if (!servoDriver.begin()) {
        DEBUG_PRINTLN("Servo driver init failed!");
        oledDisplay.showError("Servo Init Failed");
        ledIndicator.setStatus(STATUS_OVERLOAD);
        while (1) delay(100);
    }
    
    // 扫描舵机 - 循环搜索直到找到至少1个舵机
    DEBUG_PRINTLN("Scanning servos...");
    ledIndicator.setSearching(true);  // 开始搜索，LED 闪烁蓝灯
    
    int servoCount = 0;
    while (servoCount == 0) {
        // 使用回调函数扫描，实时更新显示
        servoCount = servoDriver.scanServosWithCallback(0, 10, [](int currentId, int maxId, int detected) {
            oledDisplay.showSearching(currentId, maxId, detected);
            ledIndicator.update();  // 更新 LED 闪烁
            delay(10);
        });
        
        DEBUG_PRINTF("Found %d servos\n", servoCount);
        
        if (servoCount == 0) {
            DEBUG_PRINTLN("No servos found, retrying...");
            delay(500);  // 等待500ms后重新搜索
        }
    }
    
    ledIndicator.setSearching(false);  // 搜索结束，停止闪烁
    
    // 显示搜索完成
    oledDisplay.showSearchComplete(servoCount);
    delay(1500);
    
    // 初始化 WiFi
    wifiManager.begin();
    
    // 初始化 Web 服务器
    webServer.begin();
    
    // 初始化 ESP-NOW
    espNowManager.begin();
    
    // 加载保存的模式
    loadSavedMode();
    
    DEBUG_PRINTF("Current mode: %d\n", currentMode);
    DEBUG_PRINTLN("Initialization complete!");
    
    // 进入 ESP-NOW 配对模式
    if (currentMode == MODE_LEADER || currentMode == MODE_M_LEADER) {
        // Leader 模式：扫描 Follower
        enterPairingMode();
    } else if (currentMode == MODE_FOLLOWER) {
        // Follower 模式：等待 Leader 连接
        enterFollowerMode();
    } else {
        // 其他模式直接完成初始化
        pairingCompleted = true;
        oledDisplay.showMode(currentMode);
    }
    
    ledIndicator.setMode(currentMode);
    ledIndicator.setStatus(STATUS_WAITING);
}

void loop() {
    // 处理按钮
    handleButton();
    
    // 更新 LED
    ledIndicator.update();
    
    // 如果还在配对模式，先完成配对
    if (inPairingMode) {
        // 配对模式由各自的函数处理，这里只更新显示
        if (currentMode == MODE_FOLLOWER) {
            // Follower 显示闪烁等待提示
            static uint32_t lastBlink = 0;
            if (millis() - lastBlink > 800) {
                lastBlink = millis();
                oledDisplay.showWaitingForPeer();
            }
        }
        return;  // 配对完成前不执行正常模式逻辑
    }
    
    // 处理 Web 服务器
    webServer.handleClient();
    
    // 根据模式执行不同逻辑
    switch (currentMode) {
        case MODE_FOLLOWER:
            modeFollower();
            break;
        case MODE_LEADER:
            modeLeader();
            break;
        case MODE_M_LEADER:
            modeMLeader();
            break;
        case MODE_GATEWAY:
            modeGateway();
            break;
        case MODE_JOYCON:
            modeJoyCon();
            break;
    }
    
    // 更新显示
    static uint32_t lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate > 500) {
        lastDisplayUpdate = millis();
        
        // 显示 MAC 地址和模式
        char macStr[18];
        espNowManager.getMACAddress(macStr);
        
        // 如果有配对的设备，显示配对设备的 MAC
        char peerMacStr[18] = {0};
        if (pairedPeerMAC[0] != 0xFF) {
            sprintf(peerMacStr, "%02X%02X%02X", 
                    pairedPeerMAC[3], pairedPeerMAC[4], pairedPeerMAC[5]);
        }
        
        oledDisplay.showStatus(macStr, currentMode, 
            servoDriver.getOnlineCount(), 
            ledIndicator.getStatusText(),
            peerMacStr[0] ? peerMacStr : nullptr);
    }
}

// ==================== 按钮处理 ====================
void handleButton() {
    bool buttonState = digitalRead(BOOT_BUTTON) == LOW;
    
    if (buttonState && !buttonPressed) {
        // 按钮按下
        buttonPressed = true;
        buttonPressTime = millis();
    } else if (!buttonState && buttonPressed) {
        // 按钮释放
        buttonPressed = false;
        unsigned long pressDuration = millis() - buttonPressTime;
        
        if (pressDuration >= BUTTON_LONG_PRESS_MS) {
            // 长按 - 切换模式
            switchMode();
        } else if (pressDuration >= 50 && pressDuration < BUTTON_LONG_PRESS_MS) {
            // 短按 - 设置标志
            buttonShortPress = true;
        }
    }
}

void switchMode() {
    currentMode = (DeviceMode)((currentMode + 1) % NUM_MODES);
    saveCurrentMode();
    
    DEBUG_PRINTF("Mode switched to: %d\n", currentMode);
    oledDisplay.showMode(currentMode);
    ledIndicator.setMode(currentMode);
    ledIndicator.setStatus(STATUS_WAITING);
    
    // 模式切换时重新初始化
    if (currentMode == MODE_GATEWAY) {
        espNowManager.setGatewayMode(true);
    } else {
        espNowManager.setGatewayMode(false);
    }
    
    delay(500);
}

// ==================== 模式处理 ====================
void modeFollower() {
    // 从动端模式：接收 ESP-NOW 数据并控制舵机
    
    // 检查是否收到同步数据
    if (espNowManager.hasNewPacket()) {
        ServoDataPacket packet;
        if (espNowManager.getPacket(packet)) {
            // 应用位置数据到舵机
            for (int i = 0; i < packet.servo_count && i < 12; i++) {
                uint8_t id = (packet.servo_data[i] >> 12) & 0x0F;
                uint16_t pos = packet.servo_data[i] & 0x0FFF;
                servoDriver.setPosition(id, pos, 0);  // 立即执行
            }
            
            ledIndicator.setStatus(STATUS_CONNECTED);
            
            // 发送反馈
            syncPacket.packet_type = PACKET_TYPE_STATUS;
            syncPacket.servo_count = servoDriver.getOnlineCount();
            syncPacket.timestamp = millis();
            espNowManager.sendFeedback(syncPacket);
        }
    }
    
    // 保持舵机扭矩
    servoDriver.update();
    
    delay(1);
}

void modeLeader() {
    // 主动端模式：读取舵机位置并通过 ESP-NOW 发送
    
    uint32_t now = millis();
    if (now - lastSyncTime >= SYNC_INTERVAL_MS) {
        lastSyncTime = now;
        
        // 读取所有舵机位置
        syncPacket.packet_type = PACKET_TYPE_SYNC;
        syncPacket.servo_count = 0;
        syncPacket.timestamp = now;
        
        for (uint8_t id = 1; id <= MAX_SERVO_ID && syncPacket.servo_count < 12; id++) {
            if (servoDriver.isOnline(id)) {
                uint16_t pos = servoDriver.getPosition(id);
                syncPacket.servo_data[syncPacket.servo_count] = (id << 12) | (pos & 0x0FFF);
                syncPacket.servo_count++;
            }
        }
        
        // 发送数据（如果配对了特定设备，则发送到该设备，否则广播）
        if (syncPacket.servo_count > 0) {
            if (pairedPeerMAC[0] != 0xFF) {
                // 发送到配对的设备
                espNowManager.send(pairedPeerMAC, syncPacket);
            } else {
                // 广播模式
                espNowManager.broadcast(syncPacket);
            }
            ledIndicator.setStatus(STATUS_CONNECTED);
        } else {
            ledIndicator.setStatus(STATUS_DISCONNECTED);
        }
    }
    
    // 处理 Web 命令
    webServer.processCommands(servoDriver);
    
    delay(1);
}

void modeMLeader() {
    // 多从主动端模式：广播给多个 Follower
    // 与 Leader 模式类似，但使用广播地址
    
    uint32_t now = millis();
    if (now - lastSyncTime >= SYNC_INTERVAL_MS) {
        lastSyncTime = now;
        
        // 读取所有舵机位置
        syncPacket.packet_type = PACKET_TYPE_SYNC;
        syncPacket.servo_count = 0;
        syncPacket.timestamp = now;
        
        for (uint8_t id = 1; id <= MAX_SERVO_ID && syncPacket.servo_count < 12; id++) {
            if (servoDriver.isOnline(id)) {
                uint16_t pos = servoDriver.getPosition(id);
                syncPacket.servo_data[syncPacket.servo_count] = (id << 12) | (pos & 0x0FFF);
                syncPacket.servo_count++;
            }
        }
        
        // 广播给所有 Follower（M-Leader 模式始终广播）
        if (syncPacket.servo_count > 0) {
            espNowManager.broadcast(syncPacket);
            ledIndicator.setStatus(STATUS_CONNECTED);
        }
    }
    
    delay(1);
}

void modeGateway() {
    // 网关模式：ESP-NOW ↔ 串口桥接
    
    // 处理来自 PC 的命令
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        // 解析 JSON 命令并执行
        // TODO: 实现 JSON 命令解析
    }
    
    // 转发 ESP-NOW 数据到串口
    if (espNowManager.hasNewPacket()) {
        ServoDataPacket packet;
        if (espNowManager.getPacket(packet)) {
            // 序列化为 JSON 并发送到 PC
            // TODO: 实现 JSON 序列化
        }
    }
    
    // 处理 Web 命令
    webServer.processCommands(servoDriver);
    
    delay(1);
}

void modeJoyCon() {
    // 手柄控制模式
    // TODO: 实现 Joy-Con 蓝牙连接和 IK 控制
    
    ledIndicator.setStatus(STATUS_WAITING);
    oledDisplay.showMessage("JoyCon Mode\n(Not Implemented)");
    
    delay(100);
}

// ==================== EEPROM 操作 ====================
void loadSavedMode() {
    // 从 EEPROM 加载保存的模式
    // 简化实现，实际应使用 Preferences 库
    currentMode = MODE_FOLLOWER;  // 默认从动端
}

void saveCurrentMode() {
    // 保存当前模式到 EEPROM
    // 简化实现，实际应使用 Preferences 库
}

// ==================== ESP-NOW 配对功能 ====================
void enterPairingMode() {
    inPairingMode = true;
    pairingCompleted = false;
    selectedPeerIndex = 0;
    
    // 开始扫描 ESP-NOW 设备
    espNowManager.startScanningForPeers();
    
    DEBUG_PRINTLN("Entering pairing mode...");
    oledDisplay.showMessage("Scanning peers...");
    
    // 等待发现设备（最多等待10秒）
    unsigned long scanStart = millis();
    while (millis() - scanStart < 10000) {
        handleButton();
        ledIndicator.update();
        
        // 检查是否发现设备
        if (espNowManager.hasFoundPeers()) {
            break;
        }
        
        delay(100);
    }
    
    // 如果有发现设备，进入选择流程
    if (espNowManager.hasFoundPeers()) {
        selectPeerFromList();
    } else {
        // 没有发现设备，使用广播模式
        DEBUG_PRINTLN("No peers found, using broadcast mode");
        oledDisplay.showMessage("No peers found\nBroadcast mode");
        memset(pairedPeerMAC, 0xFF, 6);  // 广播地址
        pairingCompleted = true;
        inPairingMode = false;
        delay(1500);
    }
}

void selectPeerFromList() {
    int totalPeers = espNowManager.getFoundPeerCount();
    selectedPeerIndex = 0;
    
    DEBUG_PRINTF("Found %d peers, selecting...\n", totalPeers);
    
    // 显示发现的设备列表，等待用户选择
    while (inPairingMode) {
        handleButton();
        ledIndicator.update();
        
        // 显示当前设备
        uint8_t peerMac[6];
        if (espNowManager.getFoundPeerMac(selectedPeerIndex, peerMac)) {
            char macStr[18];
            sprintf(macStr, "%02X:%02X:%02X", peerMac[3], peerMac[4], peerMac[5]);
            oledDisplay.showPairingRequest(macStr, selectedPeerIndex, totalPeers);
        }
        
        // 检查短按（确认选择）
        if (buttonShortPress) {
            buttonShortPress = false;
            
            // 确认选择当前设备
            uint8_t selectedMac[6];
            if (espNowManager.getFoundPeerMac(selectedPeerIndex, selectedMac)) {
                memcpy(pairedPeerMAC, selectedMac, 6);
                espNowManager.setTargetPeer(selectedMac);
                pairingCompleted = true;
                inPairingMode = false;
                
                char macStr[18];
                sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
                        selectedMac[0], selectedMac[1], selectedMac[2],
                        selectedMac[3], selectedMac[4], selectedMac[5]);
                DEBUG_PRINTF("Paired with: %s\n", macStr);
                
                oledDisplay.showMessage("Paired!");
                delay(1000);
            }
            break;
        }
        
        delay(100);
    }
}

void enterFollowerMode() {
    // Follower 模式：显示自己的 MAC 地址，等待 Leader 连接
    inPairingMode = true;
    pairingCompleted = false;
    
    DEBUG_PRINTLN("Entering Follower mode, waiting for Leader...");
    
    // 显示等待提示（带闪烁效果）
    unsigned long waitStart = millis();
    while (millis() - waitStart < 5000) {  // 显示5秒等待提示
        handleButton();
        ledIndicator.update();
        
        // 闪烁显示等待提示
        static bool blink = false;
        static uint32_t lastBlink = 0;
        if (millis() - lastBlink > 500) {
            lastBlink = millis();
            blink = !blink;
            
            if (blink) {
                char macStr[18];
                espNowManager.getMACAddress(macStr);
                oledDisplay.showStatus(macStr, currentMode, 
                    servoDriver.getOnlineCount(), "Wait", nullptr);
            } else {
                oledDisplay.showWaitingForPeer();
            }
        }
        
        // 检查是否收到 Leader 的数据
        if (espNowManager.hasNewPacket()) {
            // 收到数据，说明 Leader 已连接
            pairingCompleted = true;
            inPairingMode = false;
            DEBUG_PRINTLN("Leader connected!");
            oledDisplay.showMessage("Leader connected!");
            delay(1000);
            break;
        }
        
        delay(50);
    }
    
    // 5秒后自动退出配对模式，进入正常工作
    if (!pairingCompleted) {
        pairingCompleted = true;
        inPairingMode = false;
        DEBUG_PRINTLN("Auto exit pairing mode");
    }
}
