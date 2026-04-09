// web_server.h - Web 服务器头文件
#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "config.h"
#include "servo_driver.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

class WebServerManager {
public:
    WebServerManager();
    ~WebServerManager();
    
    bool begin();
    void end();
    void handleClient();
    
    // 处理命令
    void processCommands(ServoDriver& servoDriver);
    
private:
    WebServer* server;
    bool initialized;
    
    // 路由处理函数
    void handleRoot();
    void handleServoList();
    void handleServoControl();
    void handleServoStatus();
    void handleSetMode();
    void handleScan();
    void handleNotFound();
    
    // HTML 页面
    String getIndexHTML();
    String getServoControlJS();
};

#endif // WEB_SERVER_H
