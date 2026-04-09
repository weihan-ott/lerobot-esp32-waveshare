// wifi_manager.h - WiFi 管理器头文件
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "config.h"
#include <WiFi.h>

class WiFiManager {
public:
    WiFiManager();
    ~WiFiManager();
    
    bool begin();
    void end();
    
    // AP 模式
    bool startAP();
    void stopAP();
    bool isAPRunning();
    
    // 状态
    int getConnectedClients();
    String getAPIP();
    
private:
    bool apRunning;
};

#endif // WIFI_MANAGER_H
