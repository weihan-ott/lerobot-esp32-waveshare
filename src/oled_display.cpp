// oled_display.cpp - OLED 显示屏实现
#include "oled_display.h"

OLEDDisplay::OLEDDisplay() {
    mDisplay = nullptr;
    initialized = false;
}

OLEDDisplay::~OLEDDisplay() {
    if (mDisplay) {
        delete mDisplay;
        mDisplay = nullptr;
    }
}

bool OLEDDisplay::begin() {
    // 初始化 I2C
    Wire.begin(OLED_SDA, OLED_SCL);
    
    // 创建显示对象
    mDisplay = new Adafruit_SSD1306(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
    
    // 初始化显示 (地址 0x3C)
    if (!mDisplay->begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        DEBUG_PRINTLN("SSD1306 init failed");
        return false;
    }
    
    // 清空屏幕
    mDisplay->clearDisplay();
    mDisplay->setTextSize(1);
    mDisplay->setTextColor(SSD1306_WHITE);
    mDisplay->display();
    
    initialized = true;
    DEBUG_PRINTLN("OLED initialized");
    return true;
}

void OLEDDisplay::clear() {
    if (mDisplay) {
        mDisplay->clearDisplay();
    }
}

void OLEDDisplay::display() {
    if (mDisplay) {
        mDisplay->display();
    }
}

void OLEDDisplay::showStartup() {
    if (!mDisplay) return;
    
    clear();
    
    // 标题（更大字体）
    mDisplay->setTextSize(2);
    mDisplay->setCursor(10, 8);
    mDisplay->println("LeRobot");
    
    mDisplay->setTextSize(1);
    mDisplay->setCursor(20, 32);
    mDisplay->println("ESP32 Waveshare");
    
    mDisplay->setCursor(25, 48);
    mDisplay->println("Initializing...");
    
    display();
}

void OLEDDisplay::showMessage(const char* message) {
    if (!mDisplay) return;
    
    clear();
    mDisplay->setTextSize(1);
    mDisplay->setCursor(0, 0);
    mDisplay->println(message);
    display();
}

void OLEDDisplay::showError(const char* error) {
    if (!mDisplay) return;
    
    clear();
    
    // 错误图标
    mDisplay->drawRect(56, 10, 16, 16, SSD1306_WHITE);
    mDisplay->setTextSize(2);
    mDisplay->setCursor(60, 12);
    mDisplay->println("X");
    
    // 错误信息
    mDisplay->setTextSize(1);
    mDisplay->setCursor(0, 35);
    mDisplay->println("Error:");
    mDisplay->setCursor(0, 48);
    mDisplay->println(error);
    
    display();
}

void OLEDDisplay::showMode(DeviceMode mode) {
    if (!mDisplay) return;
    
    clear();
    
    mDisplay->setTextSize(2);
    drawCenteredText(10, "Mode", 2);
    
    mDisplay->setTextSize(1);
    mDisplay->drawLine(0, 28, 128, 28, SSD1306_WHITE);
    
    mDisplay->setTextSize(2);
    const char* modeName = getModeName(mode);
    int textWidth = strlen(modeName) * 12;
    int x = (128 - textWidth) / 2;
    mDisplay->setCursor(x, 40);
    mDisplay->println(modeName);
    
    display();
}

void OLEDDisplay::showStatus(const char* mac, DeviceMode mode, int servoCount, const char* status) {
    if (!mDisplay) return;
    
    clear();
    
    // 第一行：模式（居中，大字）
    mDisplay->setTextSize(2);
    const char* modeName = getModeName(mode);
    int textWidth = strlen(modeName) * 12;  // 字号2每个字符约12像素宽
    int x = (128 - textWidth) / 2;
    if (x < 0) x = 0;
    mDisplay->setCursor(x, 0);
    mDisplay->print(modeName);
    
    // 第二行：MAC 地址（居中）
    mDisplay->setTextSize(1);
    mDisplay->setCursor(0, 18);
    int macWidth = strlen(mac) * 6;
    int macX = (128 - macWidth) / 2;
    if (macX < 0) macX = 0;
    mDisplay->setCursor(macX, 18);
    mDisplay->print(mac);
    
    // 第三行：舵机数量（居中）
    mDisplay->setTextSize(1);
    char servoStr[16];
    snprintf(servoStr, sizeof(servoStr), "Servos:%d/10", servoCount);
    int servoWidth = strlen(servoStr) * 6;
    int servoX = (128 - servoWidth) / 2;
    if (servoX < 0) servoX = 0;
    mDisplay->setCursor(servoX, 34);
    mDisplay->print(servoStr);
    
    // 第四行：连接状态（居中）
    mDisplay->setTextSize(1);
    int statusWidth = strlen(status) * 6;
    int statusX = (128 - statusWidth) / 2;
    if (statusX < 0) statusX = 0;
    mDisplay->setCursor(statusX, 50);
    mDisplay->print(status);
    
    display();
}

void OLEDDisplay::drawText(int x, int y, const char* text, int size) {
    if (!mDisplay) return;
    
    mDisplay->setTextSize(size);
    mDisplay->setCursor(x, y);
    mDisplay->print(text);
}

void OLEDDisplay::drawCenteredText(int y, const char* text, int size) {
    if (!mDisplay) return;
    
    int textWidth = strlen(text) * 6 * size;
    int x = (128 - textWidth) / 2;
    if (x < 0) x = 0;
    
    drawText(x, y, text, size);
}

void OLEDDisplay::drawLine(int x0, int y0, int x1, int y1) {
    if (mDisplay) {
        mDisplay->drawLine(x0, y0, x1, y1, SSD1306_WHITE);
    }
}

void OLEDDisplay::drawRect(int x, int y, int w, int h, bool fill) {
    if (!mDisplay) return;
    
    if (fill) {
        mDisplay->fillRect(x, y, w, h, SSD1306_WHITE);
    } else {
        mDisplay->drawRect(x, y, w, h, SSD1306_WHITE);
    }
}

void OLEDDisplay::drawCircle(int x, int y, int r, bool fill) {
    if (!mDisplay) return;
    
    if (fill) {
        mDisplay->fillCircle(x, y, r, SSD1306_WHITE);
    } else {
        mDisplay->drawCircle(x, y, r, SSD1306_WHITE);
    }
}

void OLEDDisplay::drawServoPosition(int id, int position, int x, int y) {
    if (!mDisplay) return;
    
    mDisplay->setTextSize(1);
    mDisplay->setCursor(x, y);
    mDisplay->printf("%d:%d", id, position);
}

void OLEDDisplay::drawServoBar(int id, int position, int x, int y, int width) {
    if (!mDisplay) return;
    
    // ID 标签
    mDisplay->setTextSize(1);
    mDisplay->setCursor(x, y);
    mDisplay->printf("%d:", id);
    
    // 背景条
    int barX = x + 20;
    int barWidth = width - 20;
    mDisplay->drawRect(barX, y, barWidth, 6, SSD1306_WHITE);
    
    // 位置条 (映射 0-4095 到像素宽度)
    int fillWidth = (position * barWidth) / SERVO_POS_MAX;
    if (fillWidth > barWidth) fillWidth = barWidth;
    mDisplay->fillRect(barX, y, fillWidth, 6, SSD1306_WHITE);
}

const char* OLEDDisplay::getModeName(DeviceMode mode) {
    switch (mode) {
        case MODE_FOLLOWER: return "Follower";
        case MODE_LEADER:   return "Leader";
        case MODE_M_LEADER: return "M-Leader";
        case MODE_GATEWAY:  return "Gateway";
        case MODE_JOYCON:   return "JoyCon";
        default:            return "Unknown";
    }
}
