// espnow_manager.cpp - ESP-NOW 管理器实现
#include "espnow_manager.h"
#include <WiFi.h>

ESPNowManager* ESPNowManager::instance = nullptr;

ESPNowManager::ESPNowManager() {
    initialized = false;
    gatewayMode = false;
    packetReadIndex = 0;
    packetWriteIndex = 0;
    packetCount = 0;
    onDataReceived = nullptr;
    onDataSent = nullptr;
    
    memset(ownMAC, 0, 6);
    memset(targetMAC, 0xFF, 6);  // 默认广播地址
    
    instance = this;
}

ESPNowManager::~ESPNowManager() {
    end();
    instance = nullptr;
}

bool ESPNowManager::begin() {
    if (initialized) return true;
    
    // 获取本机 MAC 地址
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    memcpy(ownMAC, WiFi.macAddress().c_str(), 6);
    
    DEBUG_PRINTF("ESP-NOW MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                 ownMAC[0], ownMAC[1], ownMAC[2],
                 ownMAC[3], ownMAC[4], ownMAC[5]);
    
    // 初始化 ESP-NOW
    if (esp_now_init() != ESP_OK) {
        DEBUG_PRINTLN("ESP-NOW init failed");
        return false;
    }
    
    // 注册回调函数
    esp_now_register_recv_cb(onReceiveCallback);
    esp_now_register_send_cb(onSendCallback);
    
    // 添加广播对等点
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, ESPNOW_BROADCAST, 6);
    peerInfo.channel = ESPNOW_CHANNEL;
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        DEBUG_PRINTLN("Failed to add broadcast peer");
    }
    
    initialized = true;
    DEBUG_PRINTLN("ESP-NOW initialized");
    return true;
}

void ESPNowManager::end() {
    if (initialized) {
        esp_now_unregister_recv_cb();
        esp_now_unregister_send_cb();
        esp_now_deinit();
        initialized = false;
        DEBUG_PRINTLN("ESP-NOW deinitialized");
    }
}

void ESPNowManager::getMACAddress(char* macStr) {
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
            ownMAC[0], ownMAC[1], ownMAC[2],
            ownMAC[3], ownMAC[4], ownMAC[5]);
}

bool ESPNowManager::addPeer(const uint8_t* mac) {
    // 检查是否已存在
    if (esp_now_is_peer_exist(mac)) {
        return true;
    }
    
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = ESPNOW_CHANNEL;
    peerInfo.encrypt = false;
    
    esp_err_t result = esp_now_add_peer(&peerInfo);
    if (result == ESP_OK) {
        DEBUG_PRINTF("Peer added: %02X:%02X:%02X:%02X:%02X:%02X\n",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return true;
    }
    
    DEBUG_PRINTF("Failed to add peer, error: %d\n", result);
    return false;
}

bool ESPNowManager::removePeer(const uint8_t* mac) {
    esp_err_t result = esp_now_del_peer(mac);
    return result == ESP_OK;
}

bool ESPNowManager::setGatewayMode(bool enable) {
    gatewayMode = enable;
    DEBUG_PRINTF("Gateway mode: %s\n", enable ? "ON" : "OFF");
    return true;
}

bool ESPNowManager::send(const uint8_t* peerMac, const ServoDataPacket& packet) {
    if (!initialized) return false;
    
    // 确保对等点存在
    if (!esp_now_is_peer_exist(peerMac)) {
        if (!addPeer(peerMac)) {
            return false;
        }
    }
    
    // 发送数据
    esp_err_t result = esp_now_send(peerMac, 
                                    (uint8_t*)&packet, 
                                    sizeof(ServoDataPacket));
    
    return result == ESP_OK;
}

bool ESPNowManager::broadcast(const ServoDataPacket& packet) {
    if (!initialized) return false;
    
    esp_err_t result = esp_now_send(ESPNOW_BROADCAST, 
                                    (uint8_t*)&packet, 
                                    sizeof(ServoDataPacket));
    
    return result == ESP_OK;
}

bool ESPNowManager::sendFeedback(const ServoDataPacket& packet) {
    // 发送反馈到 Leader
    if (memcmp(targetMAC, ESPNOW_BROADCAST, 6) != 0) {
        return send(targetMAC, packet);
    }
    return false;
}

bool ESPNowManager::hasNewPacket() {
    return packetCount > 0;
}

bool ESPNowManager::getPacket(ServoDataPacket& packet) {
    return popPacket(packet);
}

void ESPNowManager::clearPacketBuffer() {
    packetReadIndex = 0;
    packetWriteIndex = 0;
    packetCount = 0;
}

uint16_t ESPNowManager::calculateCRC(const uint8_t* data, int len) {
    uint16_t crc = 0xFFFF;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

void ESPNowManager::pushPacket(const ServoDataPacket& packet) {
    if (packetCount >= PACKET_BUFFER_SIZE) {
        // 缓冲区满，丢弃最旧的数据
        packetReadIndex = (packetReadIndex + 1) % PACKET_BUFFER_SIZE;
        packetCount--;
    }
    
    packetBuffer[packetWriteIndex] = packet;
    packetWriteIndex = (packetWriteIndex + 1) % PACKET_BUFFER_SIZE;
    packetCount++;
}

bool ESPNowManager::popPacket(ServoDataPacket& packet) {
    if (packetCount == 0) {
        return false;
    }
    
    packet = packetBuffer[packetReadIndex];
    packetReadIndex = (packetReadIndex + 1) % PACKET_BUFFER_SIZE;
    packetCount--;
    return true;
}

void ESPNowManager::onReceiveCallback(const uint8_t* mac, const uint8_t* data, int len) {
    if (instance == nullptr) return;
    
    if (len != sizeof(ServoDataPacket)) {
        DEBUG_PRINTF("Invalid packet size: %d\n", len);
        return;
    }
    
    ServoDataPacket packet;
    memcpy(&packet, data, sizeof(ServoDataPacket));
    
    // 验证 CRC
    uint16_t calcCRC = instance->calculateCRC(data, len - 2);
    if (calcCRC != packet.crc) {
        DEBUG_PRINTF("CRC mismatch: calc=%04X, recv=%04X\n", calcCRC, packet.crc);
        return;
    }
    
    // 保存发送方 MAC
    memcpy(instance->targetMAC, mac, 6);
    
    // 添加到缓冲区
    instance->pushPacket(packet);
    
    // 调用用户回调
    if (instance->onDataReceived) {
        instance->onDataReceived(mac, packet);
    }
    
    DEBUG_PRINTF("Packet received from %02X:%02X:%02X:%02X:%02X:%02X, type=%d\n",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], packet.packet_type);
}

void ESPNowManager::onSendCallback(const uint8_t* mac, esp_now_send_status_t status) {
    if (instance == nullptr) return;
    
    bool success = (status == ESP_NOW_SEND_SUCCESS);
    
    DEBUG_PRINTF("Packet sent to %02X:%02X:%02X:%02X:%02X:%02X, status=%s\n",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                 success ? "OK" : "FAIL");
    
    // 调用用户回调
    if (instance->onDataSent) {
        instance->onDataSent(mac, success);
    }
}

void ESPNowManager::setOnDataReceived(OnDataReceivedCallback callback) {
    onDataReceived = callback;
}

void ESPNowManager::setOnDataSent(OnDataSentCallback callback) {
    onDataSent = callback;
}
