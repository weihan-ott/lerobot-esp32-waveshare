// led_indicator.h - LED 指示灯头文件
#ifndef LED_INDICATOR_H
#define LED_INDICATOR_H

#include "config.h"
#include <FastLED.h>

class LEDIndicator {
public:
    LEDIndicator();
    ~LEDIndicator();
    
    bool begin();
    void update();
    
    // 设置颜色
    void setColor(uint32_t color);
    void setMode(DeviceMode mode);
    void setStatus(uint32_t statusColor);
    
    // 搜索状态
    void setSearching(bool searching);
    bool isSearching();
    
    // 闪烁效果
    void blink(uint32_t color, int times = 3, int duration = 200);
    void pulse(uint32_t color, int duration = 1000);
    
    // 状态获取
    const char* getStatusText();
    
private:
    CRGB leds[NUM_LEDS];
    DeviceMode currentMode;
    uint32_t currentStatus;
    uint32_t lastUpdateTime;
    bool blinkState;
    
    // 搜索状态
    bool searching;
    bool searchBlinkState;
    uint32_t searchLastUpdate;
    
    // 动画状态
    enum AnimationState {
        ANIM_NONE,
        ANIM_BLINK,
        ANIM_PULSE,
        ANIM_RAINBOW
    };
    AnimationState animState;
    int animCount;
    int animDuration;
    uint32_t animColor;
    int animStep;
};

#endif // LED_INDICATOR_H
