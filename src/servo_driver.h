// servo_driver.h - 舵机驱动头文件
#ifndef SERVO_DRIVER_H
#define SERVO_DRIVER_H

#include "config.h"
#include <HardwareSerial.h>

// STS 协议命令
#define STS_CMD_PING        0x01
#define STS_CMD_READ        0x02
#define STS_CMD_WRITE       0x03
#define STS_CMD_REG_WRITE   0x04
#define STS_CMD_ACTION      0x05
#define STS_CMD_SYNC_WRITE  0x83

// STS 寄存器地址
#define STS_REG_ID          0x05
#define STS_REG_BAUD_RATE   0x06
#define STS_REG_TORQUE      0x28
#define STS_REG_TARGET_POS  0x2A
#define STS_REG_SPEED       0x2E
#define STS_REG_LOCK        0x30
#define STS_REG_CURRENT_POS 0x38
#define STS_REG_CURRENT_SPEED 0x3A
#define STS_REG_LOAD        0x3C
#define STS_REG_VOLTAGE     0x3E
#define STS_REG_TEMP        0x3F
#define STS_REG_MOVING      0x42

// STS 协议结构
struct STSInstructionPacket {
    uint8_t header[2];      // 0xFF 0xFF
    uint8_t id;
    uint8_t length;
    uint8_t instruction;
    uint8_t params[8];      // 最大参数数
    uint8_t checksum;
} __attribute__((packed));

struct STSStatusPacket {
    uint8_t header[2];      // 0xFF 0xFF
    uint8_t id;
    uint8_t length;
    uint8_t error;
    uint8_t params[8];      // 最大参数数
    uint8_t checksum;
} __attribute__((packed));

// 舵机状态
struct ServoStatus {
    uint8_t id;
    bool online;
    uint16_t currentPos;
    uint16_t targetPos;
    uint16_t speed;
    uint16_t load;
    uint8_t voltage;
    uint8_t temperature;
    bool moving;
    uint8_t lastError;
    uint32_t lastUpdate;
};

class ServoDriver {
public:
    ServoDriver();
    ~ServoDriver();
    
    // 初始化
    bool begin();
    void end();
    
    // 扫描舵机
    int scanServos(uint8_t startId = 1, uint8_t endId = MAX_SERVO_ID);
    
    // 带进度回调的扫描舵机
    typedef void (*ScanCallback)(int currentId, int maxId, int detected);
    int scanServosWithCallback(uint8_t startId, uint8_t endId, ScanCallback callback);
    
    // 基本操作
    bool ping(uint8_t id);
    bool setID(uint8_t oldId, uint8_t newId);
    bool setBaudRate(uint8_t id, uint32_t baud);
    
    // 位置控制
    bool setPosition(uint8_t id, uint16_t position, uint16_t speed = 0);
    bool setPositions(uint8_t ids[], uint16_t positions[], uint8_t count, uint16_t speed = 0);
    uint16_t getPosition(uint8_t id);
    
    // 速度控制
    bool setSpeed(uint8_t id, uint16_t speed);
    uint16_t getSpeed(uint8_t id);
    
    // 扭矩控制
    bool enableTorque(uint8_t id, bool enable);
    bool setTorqueLimit(uint8_t id, uint16_t limit);
    
    // 状态读取
    uint16_t getLoad(uint8_t id);
    uint8_t getVoltage(uint8_t id);
    uint8_t getTemperature(uint8_t id);
    bool isMoving(uint8_t id);
    uint8_t getError(uint8_t id);
    
    // 状态检查
    bool isOnline(uint8_t id);
    int getOnlineCount();
    ServoStatus* getStatus(uint8_t id);
    
    // 批量更新
    void update();
    void updateAllPositions();
    
    // 参数设置
    bool setAngleLimit(uint8_t id, uint16_t minAngle, uint16_t maxAngle);
    bool setVoltageLimit(uint8_t id, uint8_t minVolt, uint8_t maxVolt);
    bool setTemperatureLimit(uint8_t id, uint8_t maxTemp);
    
    // 工厂设置
    bool factoryReset(uint8_t id);
    bool reboot(uint8_t id);
    
private:
    HardwareSerial* servoSerial;
    ServoStatus servos[MAX_SERVO_ID + 1];  // ID 1-20
    
    // 通信控制
    void txEnable();
    void rxEnable();
    void flush();
    
    // 协议操作
    bool sendInstruction(uint8_t id, uint8_t instruction, uint8_t* params, uint8_t paramLen);
    bool readStatus(STSStatusPacket& status, uint32_t timeout = 100);
    uint8_t calculateChecksum(uint8_t* data, uint8_t len);
    
    // 数据转换
    uint16_t makeWord(uint8_t low, uint8_t high);
    void wordToBytes(uint16_t word, uint8_t& low, uint8_t& high);
    
    // 写寄存器
    bool writeByte(uint8_t id, uint8_t reg, uint8_t value);
    bool writeWord(uint8_t id, uint8_t reg, uint16_t value);
    
    // 读寄存器
    uint8_t readByte(uint8_t id, uint8_t reg, bool* success = nullptr);
    uint16_t readWord(uint8_t id, uint8_t reg, bool* success = nullptr);
    
    // 状态更新
    void updateServoStatus(uint8_t id, STSStatusPacket& status);
};

#endif // SERVO_DRIVER_H
