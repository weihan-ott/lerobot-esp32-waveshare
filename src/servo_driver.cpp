// servo_driver.cpp - 舵机驱动实现
#include "servo_driver.h"
#include <Arduino.h>

ServoDriver::ServoDriver() {
    servoSerial = nullptr;
    for (int i = 0; i <= MAX_SERVO_ID; i++) {
        servos[i].id = i;
        servos[i].online = false;
        servos[i].currentPos = SERVO_POS_CENTER;
        servos[i].targetPos = SERVO_POS_CENTER;
        servos[i].speed = 0;
        servos[i].load = 0;
        servos[i].voltage = 0;
        servos[i].temperature = 0;
        servos[i].moving = false;
        servos[i].lastError = 0;
        servos[i].lastUpdate = 0;
    }
}

ServoDriver::~ServoDriver() {
    end();
}

bool ServoDriver::begin() {
    // 初始化舵机串口 (UART2) - 半双工模式，1Mbps
    servoSerial = &Serial2;
    servoSerial->begin(SERVO_BAUD_RATE, SERIAL_8N1, SERVO_RX_PIN, SERVO_TX_PIN);
    
    // 配置收发控制引脚
    pinMode(SERVO_TXEN_PIN, OUTPUT);
    rxEnable();
    
    // 等待串口稳定
    delay(100);
    
    // 清空接收缓冲区
    while (servoSerial->available()) {
        servoSerial->read();
    }
    
    return true;
}

void ServoDriver::end() {
    if (servoSerial) {
        servoSerial->end();
        servoSerial = nullptr;
    }
}

void ServoDriver::txEnable() {
    digitalWrite(SERVO_TXEN_PIN, HIGH);
    delayMicroseconds(10);
}

void ServoDriver::rxEnable() {
    digitalWrite(SERVO_TXEN_PIN, LOW);
    delayMicroseconds(10);
}

void ServoDriver::flush() {
    servoSerial->flush();
    // 300μs 延迟是为了等待发送完成并切换到接收模式
    // 对于 SYNC_WRITE（广播，无响应），可以缩短延迟
    delayMicroseconds(100);
    rxEnable();
}

uint8_t ServoDriver::calculateChecksum(uint8_t* data, uint8_t len) {
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < len; i++) {
        checksum += data[i];
    }
    return ~checksum;
}

uint16_t ServoDriver::makeWord(uint8_t low, uint8_t high) {
    return (uint16_t)high << 8 | low;
}

void ServoDriver::wordToBytes(uint16_t word, uint8_t& low, uint8_t& high) {
    low = word & 0xFF;
    high = (word >> 8) & 0xFF;
}

bool ServoDriver::sendInstruction(uint8_t id, uint8_t instruction, uint8_t* params, uint8_t paramLen) {
    uint8_t packetLen = 2 + 1 + 1 + 1 + paramLen + 1;  // 包头(2) + ID(1) + 长度(1) + 指令(1) + 参数 + 校验
    uint8_t packet[64];  // 增大缓冲区以支持 SYNC_WRITE (最多12个舵机: 2+12*5+6=68, 64足够)
    uint8_t idx = 0;
    
    // 包头
    packet[idx++] = 0xFF;
    packet[idx++] = 0xFF;
    
    // ID
    packet[idx++] = id;
    
    // 长度 = 参数长度 + 2 (指令 + 校验)
    packet[idx++] = paramLen + 2;
    
    // 指令
    packet[idx++] = instruction;
    
    // 参数
    for (uint8_t i = 0; i < paramLen; i++) {
        packet[idx++] = params[i];
    }
    
    // 计算校验和 (从 ID 到参数末尾)
    packet[idx] = calculateChecksum(&packet[2], idx - 2);
    idx++;
    
    // 发送数据
    txEnable();
    servoSerial->write(packet, idx);
    flush();
    
    return true;
}

bool ServoDriver::readStatus(STSStatusPacket& status, uint32_t timeout) {
    uint32_t startTime = millis();
    uint8_t idx = 0;
    uint8_t expectedLen = 0;
    uint8_t buffer[16];
    
    while (millis() - startTime < timeout) {
        while (servoSerial->available() && idx < sizeof(buffer)) {
            buffer[idx++] = servoSerial->read();
            
            // 解析包头
            if (idx == 2) {
                if (buffer[0] != 0xFF || buffer[1] != 0xFF) {
                    // 包头错误，重新同步
                    buffer[0] = buffer[1];
                    idx = 1;
                }
            }
            
            // 获取长度
            if (idx == 4) {
                expectedLen = buffer[3] + 4;  // 长度字段 + 包头(2) + ID(1) + 长度(1)
            }
            
            // 接收完整包
            if (expectedLen > 0 && idx >= expectedLen) {
                // 复制到状态包
                status.header[0] = buffer[0];
                status.header[1] = buffer[1];
                status.id = buffer[2];
                status.length = buffer[3];
                status.error = buffer[4];
                
                uint8_t paramLen = status.length - 2;
                for (uint8_t i = 0; i < paramLen && i < 8; i++) {
                    status.params[i] = buffer[5 + i];
                }
                status.checksum = buffer[expectedLen - 1];
                
                // 验证校验和
                uint8_t calcChecksum = calculateChecksum(&buffer[2], expectedLen - 3);
                if (calcChecksum == status.checksum) {
                    return true;
                }
                return false;
            }
        }
        delayMicroseconds(100);
    }
    
    return false;
}

bool ServoDriver::ping(uint8_t id) {
    // 清空接收缓冲区
    while (servoSerial->available()) {
        servoSerial->read();
    }
    
    sendInstruction(id, STS_CMD_PING, nullptr, 0);
    
    STSStatusPacket status;
    // 增加超时时间到 200ms，兼容 STS3215
    if (readStatus(status, 200)) {
        // 只检查ID是否匹配，不检查error（有些舵机error不为0但也在线）
        if (status.id == id) {
            servos[id].online = true;
            servos[id].lastUpdate = millis();
            return true;
        }
    }
    
    servos[id].online = false;
    return false;
}

int ServoDriver::scanServos(uint8_t startId, uint8_t endId) {
    int count = 0;
    
    DEBUG_PRINTLN("Scanning servos...");
    
    for (uint8_t id = startId; id <= endId && id <= MAX_SERVO_ID; id++) {
        if (ping(id)) {
            count++;
            DEBUG_PRINTF("  Found servo ID %d\n", id);
            
            // 读取初始位置
            uint16_t pos = getPosition(id);
            servos[id].currentPos = pos;
            servos[id].targetPos = pos;
        }
        delay(5);  // 避免总线过载
    }
    
    DEBUG_PRINTF("Total servos found: %d\n", count);
    return count;
}

int ServoDriver::scanServosWithCallback(uint8_t startId, uint8_t endId, ScanCallback callback) {
    int count = 0;
    
    DEBUG_PRINTLN("Scanning servos with callback...");
    
    for (uint8_t id = startId; id <= endId && id <= MAX_SERVO_ID; id++) {
        // 调用回调函数更新显示
        if (callback) {
            callback(id, endId, count);
        }
        
        if (ping(id)) {
            count++;
            DEBUG_PRINTF("  Found servo ID %d\n", id);
            
            // 读取初始位置
            uint16_t pos = getPosition(id);
            servos[id].currentPos = pos;
            servos[id].targetPos = pos;
        }
        delay(20);  // 增加延时，给舵机足够响应时间
    }
    
    DEBUG_PRINTF("Total servos found: %d\n", count);
    return count;
}

bool ServoDriver::writeByte(uint8_t id, uint8_t reg, uint8_t value) {
    uint8_t params[2] = {reg, value};
    sendInstruction(id, STS_CMD_WRITE, params, 2);
    
    STSStatusPacket status;
    return readStatus(status, 100) && status.error == 0;
}

bool ServoDriver::writeWord(uint8_t id, uint8_t reg, uint16_t value) {
    uint8_t low, high;
    wordToBytes(value, low, high);
    uint8_t params[3] = {reg, low, high};
    sendInstruction(id, STS_CMD_WRITE, params, 3);
    
    STSStatusPacket status;
    return readStatus(status, 100) && status.error == 0;
}

uint8_t ServoDriver::readByte(uint8_t id, uint8_t reg, bool* success) {
    uint8_t params[2] = {reg, 1};  // 寄存器地址，读取长度
    sendInstruction(id, STS_CMD_READ, params, 2);
    
    STSStatusPacket status;
    // 读取1字节数据：timeout 20ms
    if (readStatus(status, 20) && status.error == 0 && status.length >= 3) {
        if (success) *success = true;
        return status.params[0];
    }
    
    if (success) *success = false;
    return 0;
}

uint16_t ServoDriver::readWord(uint8_t id, uint8_t reg, bool* success) {
    uint8_t params[2] = {reg, 2};  // 寄存器地址，读取长度
    sendInstruction(id, STS_CMD_READ, params, 2);
    
    STSStatusPacket status;
    // 读取2字节数据：length = 2(Error+1) + 2 = 4，所以 params[0] 和 params[1] 是数据
    // timeout 20ms，6个舵机最多120ms，保证30Hz刷新率
    if (readStatus(status, 20) && status.error == 0 && status.length >= 4) {
        if (success) *success = true;
        return makeWord(status.params[0], status.params[1]);
    }
    
    if (success) *success = false;
    return 0;
}

bool ServoDriver::setPosition(uint8_t id, uint16_t position, uint16_t speed) {
    if (id < 1 || id > MAX_SERVO_ID) return false;
    
    // 限制范围
    if (position > SERVO_POS_MAX) position = SERVO_POS_MAX;
    if (speed > SERVO_SPEED_MAX) speed = SERVO_SPEED_MAX;
    
    // 写入目标位置
    uint8_t lowPos, highPos, lowSpeed, highSpeed;
    wordToBytes(position, lowPos, highPos);
    wordToBytes(speed, lowSpeed, highSpeed);
    
    uint8_t params[6] = {
        STS_REG_TARGET_POS,
        lowPos, highPos,
        lowSpeed, highSpeed
    };
    
    sendInstruction(id, STS_CMD_WRITE, params, 5);
    
    STSStatusPacket status;
    if (readStatus(status, 100) && status.error == 0) {
        servos[id].targetPos = position;
        return true;
    }
    
    return false;
}

bool ServoDriver::setPositions(uint8_t ids[], uint16_t positions[], uint8_t count, uint16_t speed) {
    if (count == 0 || count > 12) return false;
    
    // 构建同步写包
    // 格式：寄存器地址(1) + 每个舵机数据长度(1) + [ID(1) + 位置(2) + 速度(2)] * count
    uint8_t params[2 + count * 5];
    uint8_t idx = 0;
    
    params[idx++] = STS_REG_TARGET_POS;  // 起始寄存器
    params[idx++] = 4;  // 每个舵机写入 4 字节（位置2 + 速度2）
    
    for (uint8_t i = 0; i < count; i++) {
        uint8_t lowPos, highPos, lowSpeed, highSpeed;
        wordToBytes(positions[i], lowPos, highPos);
        wordToBytes(speed, lowSpeed, highSpeed);
        
        params[idx++] = ids[i];
        params[idx++] = lowPos;
        params[idx++] = highPos;
        params[idx++] = lowSpeed;
        params[idx++] = highSpeed;
    }
    
    // 同步写指令发给广播地址 0xFE
    sendInstruction(0xFE, STS_CMD_SYNC_WRITE, params, idx);
    
    return true;
}

uint16_t ServoDriver::getPosition(uint8_t id) {
    bool success;
    uint16_t pos = readWord(id, STS_REG_CURRENT_POS, &success);
    
    if (success) {
        servos[id].currentPos = pos;
        servos[id].lastUpdate = millis();
        servos[id].online = true;
    } else {
        servos[id].online = false;
    }
    
    return pos;
}

bool ServoDriver::setSpeed(uint8_t id, uint16_t speed) {
    return writeWord(id, STS_REG_SPEED, speed);
}

uint16_t ServoDriver::getSpeed(uint8_t id) {
    bool success;
    return readWord(id, STS_REG_CURRENT_SPEED, &success);
}

bool ServoDriver::enableTorque(uint8_t id, bool enable) {
    return writeByte(id, STS_REG_TORQUE, enable ? 1 : 0);
}

uint16_t ServoDriver::getLoad(uint8_t id) {
    bool success;
    return readWord(id, STS_REG_LOAD, &success);
}

uint8_t ServoDriver::getVoltage(uint8_t id) {
    bool success;
    return readByte(id, STS_REG_VOLTAGE, &success);
}

uint8_t ServoDriver::getTemperature(uint8_t id) {
    bool success;
    return readByte(id, STS_REG_TEMP, &success);
}

bool ServoDriver::isMoving(uint8_t id) {
    bool success;
    return readByte(id, STS_REG_MOVING, &success) != 0;
}

uint8_t ServoDriver::getError(uint8_t id) {
    return servos[id].lastError;
}

bool ServoDriver::isOnline(uint8_t id) {
    if (id < 1 || id > MAX_SERVO_ID) return false;
    
    // 检查是否超时
    if (millis() - servos[id].lastUpdate > 1000) {
        // 超时，尝试 ping
        return ping(id);
    }
    
    return servos[id].online;
}

int ServoDriver::getOnlineCount() {
    int count = 0;
    for (int i = 1; i <= MAX_SERVO_ID; i++) {
        if (isOnline(i)) count++;
    }
    return count;
}

ServoStatus* ServoDriver::getStatus(uint8_t id) {
    if (id >= 1 && id <= MAX_SERVO_ID) {
        return &servos[id];
    }
    return nullptr;
}

void ServoDriver::update() {
    // 定期更新舵机状态
    static uint32_t lastUpdate = 0;
    
    if (millis() - lastUpdate > 100) {  // 每 100ms 更新一次
        lastUpdate = millis();
        
        for (uint8_t id = 1; id <= MAX_SERVO_ID; id++) {
            if (servos[id].online) {
                // 更新位置
                getPosition(id);
            }
        }
    }
}

void ServoDriver::updateAllPositions() {
    for (uint8_t id = 1; id <= MAX_SERVO_ID; id++) {
        if (servos[id].online) {
            getPosition(id);
        }
    }
}

bool ServoDriver::setID(uint8_t oldId, uint8_t newId) {
    return writeByte(oldId, STS_REG_ID, newId);
}

bool ServoDriver::setBaudRate(uint8_t id, uint32_t baud) {
    uint8_t baudCode;
    switch (baud) {
        case 1000000: baudCode = 1; break;
        case 500000:  baudCode = 3; break;
        case 250000:  baudCode = 7; break;
        case 128000:  baudCode = 15; break;
        case 115200:  baudCode = 16; break;
        case 57600:   baudCode = 34; break;
        case 19200:   baudCode = 103; break;
        case 9600:    baudCode = 207; break;
        default: return false;
    }
    return writeByte(id, STS_REG_BAUD_RATE, baudCode);
}

bool ServoDriver::setAngleLimit(uint8_t id, uint16_t minAngle, uint16_t maxAngle) {
    // 写入最小角度限制
    uint8_t lowMin, highMin, lowMax, highMax;
    wordToBytes(minAngle, lowMin, highMin);
    wordToBytes(maxAngle, lowMax, highMax);
    
    uint8_t params[5] = {0x0B, lowMin, highMin, lowMax, highMax};
    sendInstruction(id, STS_CMD_WRITE, params, 5);
    
    STSStatusPacket status;
    return readStatus(status, 100) && status.error == 0;
}

bool ServoDriver::setVoltageLimit(uint8_t id, uint8_t minVolt, uint8_t maxVolt) {
    uint8_t params[3] = {0x0C, minVolt, maxVolt};
    sendInstruction(id, STS_CMD_WRITE, params, 3);
    
    STSStatusPacket status;
    return readStatus(status, 100) && status.error == 0;
}

bool ServoDriver::setTemperatureLimit(uint8_t id, uint8_t maxTemp) {
    return writeByte(id, 0x0B, maxTemp);
}

bool ServoDriver::factoryReset(uint8_t id) {
    uint8_t params[1] = {0xFF};
    sendInstruction(id, 0x06, params, 1);  // RESET 指令
    
    STSStatusPacket status;
    return readStatus(status, 100) && status.error == 0;
}

bool ServoDriver::reboot(uint8_t id) {
    sendInstruction(id, 0x08, nullptr, 0);  // REBOOT 指令
    
    STSStatusPacket status;
    return readStatus(status, 100) && status.error == 0;
}

bool ServoDriver::setTorqueLimit(uint8_t id, uint16_t limit) {
    return writeWord(id, 0x10, limit);
}
