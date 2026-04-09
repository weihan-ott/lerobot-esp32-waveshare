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
    
    // 全部使用字号2
    mDisplay->setTextSize(2);
    mDisplay->setCursor(10, 0);
    mDisplay->println("LeBot");
    
    mDisplay->setCursor(0, 20);
    mDisplay->println("Waveshare");
    
    mDisplay->setCursor(0, 44);
    mDisplay->println("Start...");
    
    display();
}

void OLEDDisplay::showMessage(const char* message) {
    if (!mDisplay) return;
    
    clear();
    mDisplay->setTextSize(2);
    mDisplay->setCursor(0, 24);
    // 只显示前10个字符
    char msgShort[11];
    strncpy(msgShort, message, 10);
    msgShort[10] = '\0';
    mDisplay->println(msgShort);
    display();
}

void OLEDDisplay::showError(const char* error) {
    if (!mDisplay) return;
    
    clear();
    
    // 错误图标
    mDisplay->setTextSize(2);
    mDisplay->setCursor(50, 0);
    mDisplay->print("ERR");
    
    // 错误信息（只显示前10个字符）
    mDisplay->setCursor(0, 32);
    char errShort[11];
    strncpy(errShort, error, 10);
    errShort[10] = '\0';
    mDisplay->print(errShort);
    
    display();
}

void OLEDDisplay::showMode(DeviceMode mode) {
    if (!mDisplay) return;
    
    clear();
    
    mDisplay->setTextSize(2);
    mDisplay->setCursor(35, 0);
    mDisplay->print("MODE");
    
    mDisplay->drawLine(0, 20, 128, 20, SSD1306_WHITE);
    
    // 模式名称（截取前8个字符居中）
    const char* modeName = getModeName(mode);
    char modeShort[9];
    strncpy(modeShort, modeName, 8);
    modeShort[8] = '\0';
    int textWidth = strlen(modeShort) * 12;
    int x = (128 - textWidth) / 2;
    if (x < 0) x = 0;
    mDisplay->setCursor(x, 36);
    mDisplay->print(modeShort);
    
    display();
}

void OLEDDisplay::showStatus(const char* mac, DeviceMode mode, int servoCount, const char* status) {
    if (!mDisplay) return;
    
    clear();
    
    // 使用字号2（12x16像素），四行刚好填满64像素
    // 行高 16 像素: 0, 16, 32, 48
    mDisplay->setTextSize(2);
    
    // 第一行：模式（截取前5个字符避免超出）
    mDisplay->setCursor(0, 0);
    const char* modeName = getModeName(mode);
    char modeShort[6];
    strncpy(modeShort, modeName, 5);
    modeShort[5] = '\0';
    mDisplay->print(modeShort);
    
    // 第二行：MAC后4位
    mDisplay->setCursor(0, 16);
    int macLen = strlen(mac);
    const char* macShort = macLen > 4 ? mac + macLen - 4 : mac;
    mDisplay->print(macShort);
    
    // 第三行：舵机数量
    mDisplay->setCursor(0, 32);
    mDisplay->print("S:");
    mDisplay->print(servoCount);
    
    // 第四行：状态（前5个字符）
    mDisplay->setCursor(0, 48);
    char statusShort[6];
    strncpy(statusShort, status, 5);
    statusShort[5] = '\0';
    mDisplay->print(statusShort);
    
    display();
}

void OLEDDisplay::showSearching(int currentId, int maxId, int detected) {
    if (!mDisplay) return;
    
    clear();
    
    // 使用字号2，每行16像素高，可显示约10个字符
    mDisplay->setTextSize(2);
    
    // 第一行：Search STS
    mDisplay->setCursor(0, 0);
    mDisplay->print("SearchSTS");
    
    // 第二行：ID:当前/最大
    mDisplay->setCursor(0, 16);
    mDisplay->print("ID:");
    mDisplay->print(currentId);
    mDisplay->print("/");
    mDisplay->print(maxId);
    
    // 第三行：Found数量
    mDisplay->setCursor(0, 32);
    mDisplay->print("Found:");
    mDisplay->print(detected);
    
    display();
}

void OLEDDisplay::showSearchComplete(int detected) {
    if (!mDisplay) return;
    
    clear();
    
    mDisplay->setTextSize(2);
    
    // 居中显示完成信息
    mDisplay->setCursor(10, 8);
    mDisplay->print("Done!");
    
    mDisplay->setCursor(0, 32);
    mDisplay->print("Found:");
    mDisplay->print(detected);
    
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
