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
bool modeSelected = false;  // 模式是否已选择

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

// 启动阶段枚举
enum BootPhase {
    PHASE_SELECT_MODE = 0,    // 选择模式
    PHASE_SEARCH_SERVO,       // 搜索舵机
    PHASE_PAIRING,            // 配对（Leader/Follower）
    PHASE_RUNNING             // 正常运行
};
BootPhase currentPhase = PHASE_SELECT_MODE;

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
void selectModePhase();      // 模式选择阶段
void searchServoPhase();     // 搜索舵机阶段
void pairingPhase();         // 配对阶段
void runNormalMode();        // 正常运行阶段

void setup() {
    // 初始化调试串口
    Serial.begin(DEBUG_BAUD_RATE);
    DEBUG_PRINTLN("\n================================");
    DEBUG_PRINTLN("LeRobot ESP32 for Waveshare");
    DEBUG_PRINTLN("Booting...");
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
    delay(1000);
    
    // 初始化舵机驱动（但不搜索）
    if (!servoDriver.begin()) {
        DEBUG_PRINTLN("Servo driver init failed!");
        oledDisplay.showError("Servo Init Failed");
        ledIndicator.setStatus(STATUS_OVERLOAD);
        while (1) delay(100);
    }
    
    // 初始化 WiFi
    wifiManager.begin();
    
    // 初始化 Web 服务器
    webServer.begin();
    
    // 初始化 ESP-NOW
    espNowManager.begin();
    
    DEBUG_PRINTLN("Setup complete. Entering mode selection...");
}

void loop() {
    // 处理按钮
    handleButton();
    
    // 更新 LED
    ledIndicator.update();
    
    // 根据当前启动阶段执行不同逻辑
    switch (currentPhase) {
        case PHASE_SELECT_MODE:
            selectModePhase();
            break;
            
        case PHASE_SEARCH_SERVO:
            searchServoPhase();
            break;
            
        case PHASE_PAIRING:
            pairingPhase();
            break;
            
        case PHASE_RUNNING:
            runNormalMode();
            break;
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
        
        if (currentPhase == PHASE_SELECT_MODE) {
            // 模式选择阶段
            if (pressDuration >= BUTTON_LONG_PRESS_MS) {
                // 长按 - 确认当前模式
                modeSelected = true;
                saveCurrentMode();
                DEBUG_PRINTF("Mode confirmed: %d\n", currentMode);
            } else if (pressDuration >= 50 && pressDuration < BUTTON_LONG_PRESS_MS) {
                // 短按 - 切换到下一个模式
                currentMode = (DeviceMode)((currentMode + 1) % NUM_MODES);
                buttonShortPress = true;
                DEBUG_PRINTF("Mode switched to: %d\n", currentMode);
            }
        } else if (currentPhase == PHASE_PAIRING) {
            // 配对阶段
            if (pressDuration >= 50 && pressDuration < BUTTON_LONG_PRESS_MS) {
                // 短按 - 确认配对或切换设备
                buttonShortPress = true;
            }
        } else {
            // 运行阶段
            if (pressDuration >= BUTTON_LONG_PRESS_MS) {
                // 长按 - 重新进入模式选择
                currentPhase = PHASE_SELECT_MODE;
                modeSelected = false;
                pairingCompleted = false;
                DEBUG_PRINTLN("Re-entering mode selection");
                delay(500);
            }
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

// ==================== 启动阶段处理 ====================

void selectModePhase() {
    static DeviceMode tempMode = MODE_FOLLOWER;  // 临时模式选择
    static uint32_t lastDisplayTime = 0;
    
    // 显示当前可选模式（每秒刷新）
    if (millis() - lastDisplayTime > 200) {
        lastDisplayTime = millis();
        oledDisplay.showMode(tempMode);
        ledIndicator.setMode(tempMode);
    }
    
    // 短按：切换模式
    if (buttonShortPress) {
        buttonShortPress = false;
        tempMode = (DeviceMode)((tempMode + 1) % NUM_MODES);
        DEBUG_PRINTF("Mode selection: %d\n", tempMode);
    }
    
    // 长按：确认模式选择
    if (buttonPressTime > 0 && !buttonPressed && (millis() - buttonPressTime >= BUTTON_LONG_PRESS_MS)) {
        // 注意：这里由 handleButton 处理长按，我们只需要检测模式确定
    }
    
    // 检测长按完成（通过 handleButton 设置的模式切换）
    if (modeSelected) {
        tempMode = currentMode;  // 应用选择的模式
        DEBUG_PRINTF("Mode confirmed: %d\n", currentMode);
        
        // 进入下一阶段
        currentPhase = PHASE_SEARCH_SERVO;
        ledIndicator.setMode(currentMode);
        
        delay(500);
    }
}

void searchServoPhase() {
    static bool searchStarted = false;
    static int servoCount = 0;
    
    if (!searchStarted) {
        searchStarted = true;
        DEBUG_PRINTLN("Starting servo search...");
        ledIndicator.setSearching(true);
        servoCount = 0;
    }
    
    // 搜索舵机
    if (servoCount == 0) {
        servoCount = servoDriver.scanServosWithCallback(0, 10, [](int currentId, int maxId, int detected) {
            oledDisplay.showSearching(currentId, maxId, detected);
            ledIndicator.update();
            delay(10);
        });
        
        DEBUG_PRINTF("Found %d servos\n", servoCount);
        
        if (servoCount == 0) {
            // 没找到，显示提示并等待重试
            oledDisplay.showMessage("No servo! Retry..");
            delay(1000);
            searchStarted = false;  // 重新搜索
            return;
        }
    }
    
    // 搜索完成
    ledIndicator.setSearching(false);
    oledDisplay.showSearchComplete(servoCount);
    delay(1500);
    
    // 根据模式决定下一步
    if (currentMode == MODE_LEADER || currentMode == MODE_M_LEADER || currentMode == MODE_FOLLOWER) {
        // 需要配对的模式
        currentPhase = PHASE_PAIRING;
        inPairingMode = true;
    } else {
        // 不需要配对的模式（Gateway、JoyCon）
        currentPhase = PHASE_RUNNING;
        pairingCompleted = true;
    }
    
    searchStarted = false;  // 重置状态
}

void pairingPhase() {
    if (currentMode == MODE_LEADER || currentMode == MODE_M_LEADER) {
        // Leader 配对流程
        if (!espNowManager.isScanningForPeers()) {
            // 开始扫描
            espNowManager.startScanningForPeers();
            oledDisplay.showMessage("Scanning peers...");
            delay(500);
        }
        
        // 等待发现设备
        if (espNowManager.hasFoundPeers()) {
            // 显示发现的设备并选择
            int totalPeers = espNowManager.getFoundPeerCount();
            uint8_t peerMac[6];
            
            if (espNowManager.getFoundPeerMac(selectedPeerIndex, peerMac)) {
                char macStr[18];
                sprintf(macStr, "%02X:%02X:%02X", peerMac[3], peerMac[4], peerMac[5]);
                oledDisplay.showPairingRequest(macStr, selectedPeerIndex, totalPeers);
                
                // 短按：确认当前设备
                if (buttonShortPress) {
                    buttonShortPress = false;
                    memcpy(pairedPeerMAC, peerMac, 6);
                    espNowManager.setTargetPeer(peerMac);
                    espNowManager.stopScanningForPeers();
                    
                    pairingCompleted = true;
                    inPairingMode = false;
                    currentPhase = PHASE_RUNNING;
                    
                    oledDisplay.showMessage("Paired!");
                    delay(1000);
                    return;
                }
                
                // 长按：切换到下一个设备
                if (buttonShortPress) {  // 这里应该用另一个标志，简化处理
                    // 实际由 handleButton 处理
                }
            }
        } else {
            // 正在扫描中
            static uint32_t lastScanDisplay = 0;
            if (millis() - lastScanDisplay > 500) {
                lastScanDisplay = millis();
                oledDisplay.showMessage("Scanning...");
            }
            
            // 超时检查（10秒）
            static uint32_t scanStartTime = 0;
            if (scanStartTime == 0) scanStartTime = millis();
            
            if (millis() - scanStartTime > 10000) {
                // 超时，使用广播模式
                espNowManager.stopScanningForPeers();
                memset(pairedPeerMAC, 0xFF, 6);
                
                pairingCompleted = true;
                inPairingMode = false;
                currentPhase = PHASE_RUNNING;
                
                oledDisplay.showMessage("Broadcast mode");
                delay(1000);
                scanStartTime = 0;
            }
        }
        
    } else if (currentMode == MODE_FOLLOWER) {
        // Follower 等待连接
        static uint32_t waitStartTime = 0;
        if (waitStartTime == 0) waitStartTime = millis();
        
        // 闪烁显示等待
        static uint32_t lastBlink = 0;
        static bool blinkState = false;
        if (millis() - lastBlink > 500) {
            lastBlink = millis();
            blinkState = !blinkState;
            
            if (blinkState) {
                oledDisplay.showWaitingForPeer();
            } else {
                char macStr[18];
                espNowManager.getMACAddress(macStr);
                oledDisplay.showStatus(macStr, currentMode, 
                    servoDriver.getOnlineCount(), "Wait", nullptr);
            }
        }
        
        // 检查是否收到 Leader 数据
        if (espNowManager.hasNewPacket()) {
            pairingCompleted = true;
            inPairingMode = false;
            currentPhase = PHASE_RUNNING;
            
            oledDisplay.showMessage("Connected!");
            delay(1000);
            waitStartTime = 0;
            return;
        }
        
        // 超时（10秒）自动进入
        if (millis() - waitStartTime > 10000) {
            pairingCompleted = true;
            inPairingMode = false;
            currentPhase = PHASE_RUNNING;
            waitStartTime = 0;
        }
    }
}

void runNormalMode() {
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
        
        char macStr[18];
        espNowManager.getMACAddress(macStr);
        
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
// 旧函数已删除，使用新的启动阶段处理
