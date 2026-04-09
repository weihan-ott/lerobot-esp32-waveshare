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
    
    // 使用字号1（6x8像素），但可以稍微放大一点用 setTextSize(1)
    // 左对齐，行间距适当
    // 行高 14 像素: 0, 14, 28, 42, 56
    
    mDisplay->setTextSize(1);
    
    // 第一行：模式（字号稍微大一点）
    mDisplay->setCursor(0, 0);
    mDisplay->print("Mode:");
    mDisplay->print(getModeName(mode));
    
    // 第二行：MAC 地址
    mDisplay->setCursor(0, 14);
    mDisplay->print("MAC:");
    mDisplay->print(mac);
    
    // 第三行：舵机数量
    mDisplay->setCursor(0, 28);
    mDisplay->print("Servos:");
    mDisplay->print(servoCount);
    mDisplay->print("/10");
    
    // 第四行：连接状态
    mDisplay->setCursor(0, 42);
    mDisplay->print("Status:");
    mDisplay->print(status);
    
    display();
}

void OLEDDisplay::showSearching(int currentId, int maxId, int detected) {
    if (!mDisplay) return;
    
    clear();
    
    mDisplay->setTextSize(1);
    
    // 第一行：搜索提示
    mDisplay->setCursor(0, 0);
    mDisplay->print("Search STS/SCS...");
    
    // 第二行：搜索范围
    mDisplay->setCursor(0, 16);
    mDisplay->print("Max_ID ");
    mDisplay->print(maxId);
    mDisplay->print("-Ping 0-");
    mDisplay->print(maxId);
    
    // 第三行：当前搜索进度
    mDisplay->setCursor(0, 32);
    mDisplay->print("Checking ID:");
    mDisplay->print(currentId);
    
    // 第四行：已发现数量
    mDisplay->setCursor(0, 48);
    mDisplay->print("Detected:");
    mDisplay->print(detected);
    
    display();
}

void OLEDDisplay::showSearchComplete(int detected) {
    if (!mDisplay) return;
    
    clear();
    
    mDisplay->setTextSize(1);
    
    // 居中显示完成信息
    mDisplay->setCursor(20, 20);
    mDisplay->print("Search Complete!");
    
    mDisplay->setCursor(25, 40);
    mDisplay->print("Found ");
    mDisplay->print(detected);
    mDisplay->print(" servos");
    
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
