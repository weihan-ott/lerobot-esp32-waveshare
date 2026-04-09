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
    
    // 设置高对比度，提高清晰度
    mDisplay->ssd1306_command(SSD1306_SETCONTRAST);
    mDisplay->ssd1306_command(0xCF);  // 高对比度值 (0x00-0xFF)
    
    // 清空屏幕
    mDisplay->clearDisplay();
    mDisplay->setTextSize(1);
    mDisplay->setTextColor(SSD1306_WHITE, SSD1306_BLACK);  // 白字黑底
    mDisplay->display();
    
    initialized = true;
    DEBUG_PRINTLN("OLED initialized");
    return true;
}

void OLEDDisplay::clear() {
    if (mDisplay) {
        mDisplay->clearDisplay();
        mDisplay->setTextColor(SSD1306_WHITE);
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
    mDisplay->setFont(nullptr);
    mDisplay->setTextColor(SSD1306_WHITE, SSD1306_BLACK);

    mDisplay->setTextSize(2);
    mDisplay->setCursor(18, 12);
    mDisplay->print("LeRobot");

    mDisplay->setTextSize(1);
    mDisplay->setCursor(28, 36);
    mDisplay->print("ESP32 Waveshare");

    mDisplay->setCursor(36, 52);
    mDisplay->print("Starting...");

    display();
}

void OLEDDisplay::showMessage(const char* message) {
    if (!mDisplay) return;
    
    clear();
    mDisplay->setFont(nullptr);
    mDisplay->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    mDisplay->setTextSize(1);
    mDisplay->setCursor(0, 28);
    mDisplay->print(message);
    display();
}

void OLEDDisplay::showError(const char* error) {
    if (!mDisplay) return;
    
    clear();
    
    mDisplay->setFont(nullptr);
    mDisplay->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    mDisplay->setTextSize(2);
    mDisplay->setCursor(18, 8);
    mDisplay->print("ERROR");

    mDisplay->setTextSize(1);
    mDisplay->setCursor(0, 40);
    mDisplay->print(error);
    
    display();
}

void OLEDDisplay::showMode(DeviceMode mode) {
    if (!mDisplay) return;
    
    clear();
    
    mDisplay->setFont(nullptr);
    mDisplay->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    mDisplay->setTextSize(1);
    mDisplay->setCursor(40, 8);
    mDisplay->print("MODE");

    mDisplay->drawLine(18, 18, 110, 18, SSD1306_WHITE);

    mDisplay->setTextSize(2);
    int nameLen = strlen(getModeName(mode));
    int textX = (OLED_WIDTH - nameLen * 12) / 2;
    if (textX < 0) textX = 0;
    mDisplay->setCursor(textX, 32);
    mDisplay->print(getModeName(mode));
    
    display();
}

void OLEDDisplay::showStatus(const char* mac, DeviceMode mode, int servoCount, const char* status) {
    if (!mDisplay) return;
    
    clear();
    
    mDisplay->setFont(nullptr);
    mDisplay->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    
    // 第一行：模式名称（字号2，清晰大字）
    mDisplay->setTextSize(2);
    const char* modeName = getModeName(mode);
    int nameLen = strlen(modeName);
    int textX = (OLED_WIDTH - nameLen * 12) / 2;  // 居中
    if (textX < 0) textX = 0;
    mDisplay->setCursor(textX, 0);
    mDisplay->print(modeName);

    // 分隔线
    mDisplay->drawLine(0, 20, OLED_WIDTH, 20, SSD1306_WHITE);

    // MAC 只显示后 6 个字符
    char macShort[10] = {0};
    int macLen = strlen(mac);
    if (macLen > 6) {
        strcpy(macShort, mac + macLen - 6);
    } else {
        strcpy(macShort, mac);
    }

    // 第二行：MAC 和 Servos
    mDisplay->setTextSize(1);
    mDisplay->setCursor(0, 28);
    mDisplay->print("MAC:");
    mDisplay->print(macShort);
    
    mDisplay->setCursor(70, 28);
    mDisplay->print("S:");
    mDisplay->print(servoCount);

    // 第三行：状态
    mDisplay->setCursor(0, 44);
    mDisplay->print("Stat:");
    mDisplay->print(status);
    
    display();
}

void OLEDDisplay::showSearching(int currentId, int maxId, int detected) {
    if (!mDisplay) return;
    
    clear();
    
    mDisplay->setFont(nullptr);
    mDisplay->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    mDisplay->setTextSize(1);
    
    mDisplay->setCursor(0, 0);
    mDisplay->print("Search STS/SCS...");
    
    mDisplay->setCursor(0, 16);
    mDisplay->print("MAX_ID ");
    mDisplay->print(maxId);
    mDisplay->print("-Ping:");
    mDisplay->print(currentId);
    
    mDisplay->setCursor(0, 32);
    mDisplay->print("Detected:");
    mDisplay->print(detected);
    
    display();
}

void OLEDDisplay::showSearchComplete(int detected) {
    if (!mDisplay) return;
    
    clear();
    
    mDisplay->setFont(nullptr);
    mDisplay->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    mDisplay->setTextSize(2);
    mDisplay->setCursor(10, 16);
    mDisplay->print("Done");

    mDisplay->setTextSize(1);
    mDisplay->setCursor(0, 40);
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
    mDisplay->setTextSize(size);
    int textWidth = strlen(text) * 6 * size;
    int x = (OLED_WIDTH - textWidth) / 2;
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
    mDisplay->setTextSize(1);
    // ID 标签
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
