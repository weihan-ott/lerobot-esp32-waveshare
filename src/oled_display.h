// oled_display.h - OLED 显示屏头文件
#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include "config.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// 使用更清晰的字体 (细体)
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>

class OLEDDisplay {
public:
    OLEDDisplay();
    ~OLEDDisplay();
    
    bool begin();
    void clear();
    void display();
    
    // 显示画面
    void showStartup();
    void showMessage(const char* message);
    void showError(const char* error);
    void showMode(DeviceMode mode);
    void showStatus(const char* mac, DeviceMode mode, int servoCount, const char* status);
    
    // 搜索舵机显示
    void showSearching(int currentId, int maxId, int detected);
    void showSearchComplete(int detected);
    
    // 基本绘图
    void drawText(int x, int y, const char* text, int size = 1);
    void drawCenteredText(int y, const char* text, int size = 1);
    void drawLine(int x0, int y0, int x1, int y1);
    void drawRect(int x, int y, int w, int h, bool fill = false);
    void drawCircle(int x, int y, int r, bool fill = false);
    
    // 舵机显示
    void drawServoPosition(int id, int position, int x, int y);
    void drawServoBar(int id, int position, int x, int y, int width = 100);
    
private:
    Adafruit_SSD1306* mDisplay;
    bool initialized;
    
    const char* getModeName(DeviceMode mode);
};

#endif // OLED_DISPLAY_H
