// espnow_manager.h - ESP-NOW 管理器头文件
#ifndef ESPNOW_MANAGER_H
#define ESPNOW_MANAGER_H

#include "config.h"
#include <esp_now.h>

// ESP-NOW 数据包结构
struct ServoDataPacket {
    uint8_t  packet_type;      // 数据包类型
    uint8_t  servo_count;      // 舵机数量
    uint16_t servo_data[12];   // 每个舵机：高4位ID + 低12位位置
    uint32_t timestamp;        // 时间戳
    uint16_t crc;              // CRC 校验
} __attribute__((packed));

// 回调函数类型定义
typedef void (*OnDataReceivedCallback)(const uint8_t* mac, const ServoDataPacket& packet);
typedef void (*OnDataSentCallback)(const uint8_t* mac, bool success);

class ESPNowManager {
public:
    ESPNowManager();
    ~ESPNowManager();
    
    bool begin();
    void end();
    
    // 发送数据
    bool send(const uint8_t* peerMac, const ServoDataPacket& packet);
    bool broadcast(const ServoDataPacket& packet);
    bool sendFeedback(const ServoDataPacket& packet);
    
    // 接收数据
    bool hasNewPacket();
    bool getPacket(ServoDataPacket& packet);
    void clearPacketBuffer();
    
    // 设备管理
    bool addPeer(const uint8_t* mac);
    bool removePeer(const uint8_t* mac);
    bool setGatewayMode(bool enable);
    
    // MAC 地址
    void getMACAddress(char* macStr);
    uint8_t* getOwnMAC() { return ownMAC; }
    
    // 回调设置
    void setOnDataReceived(OnDataReceivedCallback callback);
    void setOnDataSent(OnDataSentCallback callback);
    
private:
    bool initialized;
    bool gatewayMode;
    uint8_t ownMAC[6];
    uint8_t targetMAC[6];
    
    // 接收缓冲区
    static const int PACKET_BUFFER_SIZE = 10;
    ServoDataPacket packetBuffer[PACKET_BUFFER_SIZE];
    int packetReadIndex;
    int packetWriteIndex;
    int packetCount;
    
    // 回调
    OnDataReceivedCallback onDataReceived;
    OnDataSentCallback onDataSent;
    
    // ESP-NOW 回调函数
    static void onReceiveCallback(const uint8_t* mac, const uint8_t* data, int len);
    static void onSendCallback(const uint8_t* mac, esp_now_send_status_t status);
    
    // 静态实例指针（用于回调）
    static ESPNowManager* instance;
    
    // 辅助函数
    uint16_t calculateCRC(const uint8_t* data, int len);
    void pushPacket(const ServoDataPacket& packet);
    bool popPacket(ServoDataPacket& packet);
};

#endif // ESPNOW_MANAGER_H
