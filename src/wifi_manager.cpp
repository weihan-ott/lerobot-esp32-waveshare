// wifi_manager.cpp - WiFi 管理器实现
#include "wifi_manager.h"

WiFiManager::WiFiManager() {
    apRunning = false;
}

WiFiManager::~WiFiManager() {
    end();
}

bool WiFiManager::begin() {
    // WiFi 模式在 ESP-NOW 初始化时已经设置
    // 这里只需要确保模式正确
    if (WiFi.getMode() == WIFI_MODE_NULL) {
        WiFi.mode(WIFI_STA);
    }
    
    DEBUG_PRINTLN("WiFi manager initialized");
    return true;
}

void WiFiManager::end() {
    stopAP();
    WiFi.mode(WIFI_OFF);
}

bool WiFiManager::startAP() {
    if (apRunning) return true;
    
    // 设置 AP 模式
    WiFi.mode(WIFI_AP_STA);
    
    // 配置 AP
    IPAddress localIP(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    
    WiFi.softAPConfig(localIP, gateway, subnet);
    
    // 启动 AP
    bool result = WiFi.softAP(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
    
    if (result) {
        apRunning = true;
        DEBUG_PRINTF("AP started: %s\n", WIFI_SSID);
        DEBUG_PRINTF("AP IP: %s\n", getAPIP().c_str());
    } else {
        DEBUG_PRINTLN("Failed to start AP");
    }
    
    return result;
}

void WiFiManager::stopAP() {
    if (apRunning) {
        WiFi.softAPdisconnect(true);
        apRunning = false;
        DEBUG_PRINTLN("AP stopped");
    }
}

bool WiFiManager::isAPRunning() {
    return apRunning;
}

int WiFiManager::getConnectedClients() {
    if (!apRunning) return 0;
    return WiFi.softAPgetStationNum();
}

String WiFiManager::getAPIP() {
    return WiFi.softAPIP().toString();
}
