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
    
    // 使用粗体字体
    mDisplay->setFont(&FreeSansBold9pt7b);
    mDisplay->setTextSize(1);
    
    mDisplay->setCursor(15, 18);
    mDisplay->print("LeRobot");
    
    mDisplay->setFont(&FreeSans9pt7b);
    mDisplay->setCursor(5, 40);
    mDisplay->print("Waveshare");
    
    mDisplay->setCursor(20, 58);
    mDisplay->print("Starting");
    
    // 恢复默认字体
    mDisplay->setFont(nullptr);
    display();
}

void OLEDDisplay::showMessage(const char* message) {
    if (!mDisplay) return;
    
    clear();
    mDisplay->setFont(&FreeSans9pt7b);
    mDisplay->setTextSize(1);
    mDisplay->setCursor(0, 35);
    mDisplay->print(message);
    // 恢复默认字体
    mDisplay->setFont(nullptr);
    display();
}

void OLEDDisplay::showError(const char* error) {
    if (!mDisplay) return;
    
    clear();
    
    // 错误标题
    mDisplay->setFont(&FreeSansBold9pt7b);
    mDisplay->setTextSize(1);
    mDisplay->setCursor(35, 18);
    mDisplay->print("ERROR");
    
    // 错误信息
    mDisplay->setFont(&FreeSans9pt7b);
    mDisplay->setCursor(0, 45);
    mDisplay->print(error);
    
    // 恢复默认字体
    mDisplay->setFont(nullptr);
    display();
}

void OLEDDisplay::showMode(DeviceMode mode) {
    if (!mDisplay) return;
    
    clear();
    
    mDisplay->setFont(&FreeSansBold9pt7b);
    mDisplay->setTextSize(1);
    mDisplay->setCursor(35, 18);
    mDisplay->print("MODE");
    
    mDisplay->drawLine(10, 24, 118, 24, SSD1306_WHITE);
    
    // 模式名称
    mDisplay->setFont(&FreeSans9pt7b);
    mDisplay->setCursor(20, 50);
    mDisplay->print(getModeName(mode));
    
    // 恢复默认字体
    mDisplay->setFont(nullptr);
    display();
}

void OLEDDisplay::showStatus(const char* mac, DeviceMode mode, int servoCount, const char* status) {
    if (!mDisplay) return;
    
    clear();
    
    // 使用 FreeSans 字体，清晰且完整显示
    mDisplay->setFont(&FreeSans9pt7b);
    mDisplay->setTextSize(1);
    
    // 第一行：Mode:Leader
    mDisplay->setCursor(0, 12);
    mDisplay->print("Mode:");
    mDisplay->print(getModeName(mode));
    
    // 第二行：MAC地址（显示后8位）
    mDisplay->setCursor(0, 28);
    mDisplay->print("MAC:");
    int macLen = strlen(mac);
    const char* macShort = macLen > 8 ? mac + macLen - 8 : mac;
    mDisplay->print(macShort);
    
    // 第三行：Servos:X/10
    mDisplay->setCursor(0, 44);
    mDisplay->print("Servos:");
    mDisplay->print(servoCount);
    mDisplay->print("/10");
    
    // 第四行：Status:XXX
    mDisplay->setCursor(0, 60);
    mDisplay->print("Stat:");
    mDisplay->print(status);
    
    // 恢复默认字体
    mDisplay->setFont(nullptr);
    display();
}

void OLEDDisplay::showSearching(int currentId, int maxId, int detected) {
    if (!mDisplay) return;
    
    clear();
    
    // 使用 FreeSans 字体，更清晰
    mDisplay->setFont(&FreeSans9pt7b);
    mDisplay->setTextSize(1);
    
    // 第一行：Search STS/SCS
    mDisplay->setCursor(0, 12);
    mDisplay->print("Search STS/SCS");
    
    // 第二行：Max_ID 10-Ping
    mDisplay->setCursor(0, 28);
    mDisplay->print("Max_ID ");
    mDisplay->print(maxId);
    mDisplay->print("-Ping");
    
    // 第三行：Checking ID:X
    mDisplay->setCursor(0, 44);
    mDisplay->print("ID:");
    mDisplay->print(currentId);
    mDisplay->print(" Det:");
    mDisplay->print(detected);
    
    // 恢复默认字体
    mDisplay->setFont(nullptr);
    display();
}

void OLEDDisplay::showSearchComplete(int detected) {
    if (!mDisplay) return;
    
    clear();
    
    mDisplay->setFont(&FreeSansBold9pt7b);
    mDisplay->setTextSize(1);
    
    // 居中显示完成信息
    mDisplay->setCursor(10, 25);
    mDisplay->print("Search Done!");
    
    mDisplay->setFont(&FreeSans9pt7b);
    mDisplay->setCursor(15, 50);
    mDisplay->print("Found ");
    mDisplay->print(detected);
    
    // 恢复默认字体
    mDisplay->setFont(nullptr);
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
