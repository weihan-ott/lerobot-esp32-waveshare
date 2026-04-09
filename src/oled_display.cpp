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
    
    // 使用默认字体，字号1（细体清晰）
    mDisplay->setFont(nullptr);
    mDisplay->setTextSize(1);
    
    mDisplay->setCursor(30, 16);
    mDisplay->print("LeRobot");
    
    mDisplay->setCursor(20, 32);
    mDisplay->print("Waveshare");
    
    mDisplay->setCursor(25, 48);
    mDisplay->print("Starting...");
    
    display();
}

void OLEDDisplay::showMessage(const char* message) {
    if (!mDisplay) return;
    
    clear();
    mDisplay->setFont(nullptr);
    mDisplay->setTextSize(1);
    mDisplay->setCursor(0, 32);
    mDisplay->print(message);
    display();
}

void OLEDDisplay::showError(const char* error) {
    if (!mDisplay) return;
    
    clear();
    
    mDisplay->setFont(nullptr);
    mDisplay->setTextSize(1);
    
    // 错误标题
    mDisplay->setCursor(45, 16);
    mDisplay->print("ERROR");
    
    // 错误信息
    mDisplay->setCursor(0, 45);
    mDisplay->print(error);
    
    display();
}

void OLEDDisplay::showMode(DeviceMode mode) {
    if (!mDisplay) return;
    
    clear();
    
    mDisplay->setFont(nullptr);
    mDisplay->setTextSize(1);
    mDisplay->setCursor(45, 16);
    mDisplay->print("MODE");
    
    mDisplay->drawLine(20, 20, 108, 20, SSD1306_WHITE);
    
    // 模式名称
    mDisplay->setCursor(30, 45);
    mDisplay->print(getModeName(mode));
    
    display();
}

void OLEDDisplay::showStatus(const char* mac, DeviceMode mode, int servoCount, const char* status) {
    if (!mDisplay) return;
    
    clear();
    
    // 使用默认字体，细体清晰
    mDisplay->setFont(nullptr);
    mDisplay->setTextSize(1);
    
    // 第一行：Mode:Leader
    mDisplay->setCursor(0, 10);
    mDisplay->print("Mode:");
    mDisplay->print(getModeName(mode));
    
    // 第二行：MAC地址（完整显示）
    mDisplay->setCursor(0, 24);
    mDisplay->print("MAC:");
    mDisplay->print(mac);
    
    // 第三行：Servos:X/10
    mDisplay->setCursor(0, 38);
    mDisplay->print("Servos:");
    mDisplay->print(servoCount);
    mDisplay->print("/10");
    
    // 第四行：Status:XXX
    mDisplay->setCursor(0, 52);
    mDisplay->print("Status:");
    mDisplay->print(status);
    
    display();
}

void OLEDDisplay::showSearching(int currentId, int maxId, int detected) {
    if (!mDisplay) return;
    
    clear();
    
    // 使用默认字体，细体清晰
    mDisplay->setFont(nullptr);
    mDisplay->setTextSize(1);
    
    // 第一行：Search STS/SCS...
    mDisplay->setCursor(0, 10);
    mDisplay->print("Search STS/SCS...");
    
    // 第二行：Max_ID X-Ping:0-X
    mDisplay->setCursor(0, 24);
    mDisplay->print("MAX_ID ");
    mDisplay->print(maxId);
    mDisplay->print("-Ping:0-");
    mDisplay->print(maxId);
    
    // 第三行：Checking ID:X
    mDisplay->setCursor(0, 38);
    mDisplay->print("Checking ID:");
    mDisplay->print(currentId);
    
    // 第四行：Detected:X
    mDisplay->setCursor(0, 52);
    mDisplay->print("Detected:");
    mDisplay->print(detected);
    
    display();
}

void OLEDDisplay::showSearchComplete(int detected) {
    if (!mDisplay) return;
    
    clear();
    
    mDisplay->setFont(nullptr);
    mDisplay->setTextSize(1);
    
    // 居中显示完成信息
    mDisplay->setCursor(20, 25);
    mDisplay->print("Search Done!");
    
    mDisplay->setCursor(25, 45);
    mDisplay->print("Found ");
    mDisplay->print(detected);
    mDisplay->print(" servos");
    
    display();
}

void OLEDDisplay::drawText(int x, int y, const char* text, int size) {
    if (!mDisplay) return;
    
    mDisplay->setFont(nullptr);
    mDisplay->setTextSize(size);
    mDisplay->setCursor(x, y);
    mDisplay->print(text);
}

void OLEDDisplay::drawCenteredText(int y, const char* text, int size) {
    if (!mDisplay) return;
    
    mDisplay->setFont(nullptr);
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
    
    mDisplay->setFont(nullptr);
    mDisplay->setTextSize(1);
    mDisplay->setCursor(x, y);
    mDisplay->printf("%d:%d", id, position);
}

void OLEDDisplay::drawServoBar(int id, int position, int x, int y, int width) {
    if (!mDisplay) return;
    
    mDisplay->setFont(nullptr);
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
