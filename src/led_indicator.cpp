// led_indicator.cpp - LED 指示灯实现
#include "led_indicator.h"

LEDIndicator::LEDIndicator() {
    currentMode = MODE_FOLLOWER;
    currentStatus = STATUS_WAITING;
    lastUpdateTime = 0;
    blinkState = false;
    animState = ANIM_NONE;
    animCount = 0;
    animDuration = 0;
    animColor = 0;
    animStep = 0;
    searching = false;
    searchBlinkState = false;
}

LEDIndicator::~LEDIndicator() {
}

bool LEDIndicator::begin() {
    // 初始化 FastLED
    FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(50);  // 设置亮度 (0-255)
    
    // 初始状态
    setColor(0x000000);  // 关闭
    
    DEBUG_PRINTLN("LED initialized");
    return true;
}

void LEDIndicator::update() {
    uint32_t now = millis();
    
    // 搜索状态：蓝色闪烁（最高优先级）
    if (searching) {
        if (now - searchLastUpdate >= 200) {  // 200ms 闪烁周期
            searchLastUpdate = now;
            searchBlinkState = !searchBlinkState;
            
            if (searchBlinkState) {
                leds[0] = CRGB::Blue;  // 蓝色亮
            } else {
                leds[0] = CRGB::Black;  // 灭
            }
            FastLED.show();
        }
        return;  // 搜索状态下不执行其他动画
    }
    
    // 处理动画
    switch (animState) {
        case ANIM_BLINK:
            if (now - lastUpdateTime >= animDuration) {
                lastUpdateTime = now;
                blinkState = !blinkState;
                
                if (blinkState) {
                    leds[0] = CRGB((animColor >> 16) & 0xFF, 
                                   (animColor >> 8) & 0xFF, 
                                   animColor & 0xFF);
                } else {
                    leds[0] = CRGB::Black;
                }
                
                FastLED.show();
                animStep++;
                
                if (animStep >= animCount * 2) {
                    animState = ANIM_NONE;
                    // 恢复状态颜色
                    setStatus(currentStatus);
                }
            }
            break;
            
        case ANIM_PULSE:
            if (now - lastUpdateTime >= 50) {  // 20fps
                lastUpdateTime = now;
                
                // 脉冲效果：亮度从 0 到 255 再回到 0
                float phase = (float)(now % animDuration) / animDuration;
                uint8_t brightness = (uint8_t)(sin(phase * PI * 2) * 127 + 128);
                
                leds[0] = CRGB((animColor >> 16) & 0xFF, 
                               (animColor >> 8) & 0xFF, 
                               animColor & 0xFF);
                leds[0].nscale8(brightness);
                
                FastLED.show();
            }
            break;
            
        case ANIM_RAINBOW:
            if (now - lastUpdateTime >= 50) {
                lastUpdateTime = now;
                
                uint8_t hue = (now / 20) % 256;  // 循环颜色
                leds[0] = CHSV(hue, 255, 255);
                FastLED.show();
            }
            break;
            
        default:
            // 静态显示，使用模式颜色
            // 每 500ms 闪烁一次状态
            if ((currentStatus == STATUS_DISCONNECTED || currentStatus == STATUS_PENDING) &&
                now - lastUpdateTime >= 500) {
                lastUpdateTime = now;
                blinkState = !blinkState;
                
                if (blinkState) {
                    leds[0] = CRGB((currentStatus >> 16) & 0xFF, 
                                   (currentStatus >> 8) & 0xFF, 
                                   currentStatus & 0xFF);
                } else {
                    // 显示模式颜色
                    uint32_t modeColor = MODE_COLORS[currentMode];
                    leds[0] = CRGB((modeColor >> 16) & 0xFF, 
                                   (modeColor >> 8) & 0xFF, 
                                   modeColor & 0xFF);
                }
                FastLED.show();
            } else if (currentStatus == STATUS_OVERLOAD) {
                // 过载：快速红色闪烁
                if (now - lastUpdateTime >= 100) {
                    lastUpdateTime = now;
                    blinkState = !blinkState;
                    
                    if (blinkState) {
                        leds[0] = CRGB::Red;
                    } else {
                        leds[0] = CRGB::Black;
                    }
                    FastLED.show();
                }
            }
            break;
    }
}

void LEDIndicator::setColor(uint32_t color) {
    leds[0] = CRGB((color >> 16) & 0xFF, 
                   (color >> 8) & 0xFF, 
                   color & 0xFF);
    FastLED.show();
    
    // 停止动画
    animState = ANIM_NONE;
}

void LEDIndicator::setMode(DeviceMode mode) {
    currentMode = mode;
    
    // 设置模式颜色
    uint32_t modeColor = MODE_COLORS[mode];
    setColor(modeColor);
    
    DEBUG_PRINTF("LED Mode set: %d, Color: %06X\n", mode, modeColor);
}

void LEDIndicator::setStatus(uint32_t statusColor) {
    currentStatus = statusColor;
    
    // 根据状态设置显示
    switch (statusColor) {
        case STATUS_CONNECTED:
            // 已连接：常亮绿色
            setColor(0x00FF00);
            break;
            
        case STATUS_WAITING:
            // 等待中：蓝色常亮
            setColor(0x0000FF);
            break;
            
        case STATUS_TAKEOVER:
            // 被接管：紫色常亮
            setColor(0x800080);
            break;
            
        case STATUS_DISCONNECTED:
        case STATUS_PENDING:
            // 这些状态会触发闪烁动画
            // 在 update() 中处理
            break;
            
        case STATUS_OVERLOAD:
            // 过载：快速闪烁
            // 在 update() 中处理
            break;
            
        default:
            // 其他状态使用模式颜色
            setColor(MODE_COLORS[currentMode]);
            break;
    }
    
    DEBUG_PRINTF("LED Status set: %06X\n", statusColor);
}

void LEDIndicator::blink(uint32_t color, int times, int duration) {
    animState = ANIM_BLINK;
    animColor = color;
    animCount = times;
    animDuration = duration;
    animStep = 0;
    lastUpdateTime = millis();
    blinkState = false;
    
    DEBUG_PRINTF("LED Blink: color=%06X, times=%d, duration=%d\n", color, times, duration);
}

void LEDIndicator::pulse(uint32_t color, int duration) {
    animState = ANIM_PULSE;
    animColor = color;
    animDuration = duration;
    lastUpdateTime = millis();
    
    DEBUG_PRINTF("LED Pulse: color=%06X, duration=%d\n", color, duration);
}

const char* LEDIndicator::getStatusText() {
    if (currentStatus == STATUS_CONNECTED) return "OK";
    if (currentStatus == STATUS_WAITING) return "WAIT";
    if (currentStatus == STATUS_DISCONNECTED) return "NOCONN";
    if (currentStatus == STATUS_PENDING) return "PEND";
    if (currentStatus == STATUS_TAKEOVER) return "PC";
    if (currentStatus == STATUS_OVERLOAD) return "OVER";
    return "--";
}

void LEDIndicator::setSearching(bool searchState) {
    searching = searchState;
    if (searching) {
        searchBlinkState = false;
        searchLastUpdate = millis();
    }
}

bool LEDIndicator::isSearching() {
    return searching;
}
