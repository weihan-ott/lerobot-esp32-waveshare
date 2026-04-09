#!/usr/bin/env python3
"""
LeRobot ESP32 客户端
用于 PC 与 ESP32 Gateway 通信
"""

import serial
import json
import struct
import time
import threading
from typing import Dict, List, Optional, Callable
from dataclasses import dataclass


@dataclass
class ServoStatus:
    """舵机状态"""
    id: int
    position: int
    load: int = 0
    voltage: float = 0.0
    temperature: int = 0
    moving: bool = False


class LeRobotClient:
    """LeRobot ESP32 客户端"""
    
    def __init__(self, port: str = None, baudrate: int = 1000000):
        self.port = port
        self.baudrate = baudrate
        self.serial_port: Optional[serial.Serial] = None
        self.connected = False
        self.running = False
        self.thread: Optional[threading.Thread] = None
        
        # 状态数据
        self.servo_positions: Dict[int, int] = {}
        self.servo_status: Dict[int, ServoStatus] = {}
        
        # 回调函数
        self.on_data_received: Optional[Callable] = None
        
    def connect(self, port: str = None) -> bool:
        """连接到 ESP32"""
        if port:
            self.port = port
            
        if not self.port:
            raise ValueError("Serial port not specified")
            
        try:
            self.serial_port = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.1
            )
            
            # 清空缓冲区
            self.serial_port.reset_input_buffer()
            self.serial_port.reset_output_buffer()
            
            self.connected = True
            self.running = True
            
            # 启动接收线程
            self.thread = threading.Thread(target=self._receive_loop, daemon=True)
            self.thread.start()
            
            print(f"Connected to {self.port} at {self.baudrate} baud")
            return True
            
        except Exception as e:
            print(f"Failed to connect: {e}")
            return False
    
    def disconnect(self):
        """断开连接"""
        self.running = False
        
        if self.thread:
            self.thread.join(timeout=1.0)
            
        if self.serial_port:
            self.serial_port.close()
            self.serial_port = None
            
        self.connected = False
        print("Disconnected")
        
    def _receive_loop(self):
        """接收数据循环"""
        buffer = ""
        
        while self.running:
            try:
                if self.serial_port and self.serial_port.in_waiting:
                    data = self.serial_port.read(self.serial_port.in_waiting)
                    buffer += data.decode('utf-8', errors='ignore')
                    
                    # 处理 JSON 行
                    while '\n' in buffer:
                        line, buffer = buffer.split('\n', 1)
                        line = line.strip()
                        if line:
                            self._handle_message(line)
                            
            except Exception as e:
                if self.running:
                    print(f"Receive error: {e}")
                    
            time.sleep(0.001)
            
    def _handle_message(self, message: str):
        """处理接收到的消息"""
        try:
            data = json.loads(message)
            msg_type = data.get('type')
            
            if msg_type == 'positions':
                # 更新位置数据
                for servo_data in data.get('data', []):
                    sid = servo_data.get('id')
                    pos = servo_data.get('pos')
                    if sid is not None and pos is not None:
                        self.servo_positions[sid] = pos
                        
            elif msg_type == 'status':
                # 更新状态
                pass
                
            # 调用用户回调
            if self.on_data_received:
                self.on_data_received(data)
                
        except json.JSONDecodeError:
            pass
            
    def send_command(self, cmd: Dict) -> bool:
        """发送命令"""
        if not self.serial_port:
            return False
            
        try:
            message = json.dumps(cmd) + '\n'
            self.serial_port.write(message.encode('utf-8'))
            self.serial_port.flush()
            return True
        except Exception as e:
            print(f"Send error: {e}")
            return False
            
    def set_position(self, servo_id: int, position: int, speed: int = 500) -> bool:
        """设置单个舵机位置"""
        return self.send_command({
            'cmd': 'set_position',
            'id': servo_id,
            'pos': position,
            'speed': speed
        })
        
    def set_positions(self, positions: Dict[int, int], speed: int = 500) -> bool:
        """批量设置舵机位置"""
        servo_list = []
        for sid, pos in positions.items():
            servo_list.append({'id': sid, 'pos': pos, 'speed': speed})
            
        return self.send_command({
            'cmd': 'set_positions',
            'servos': servo_list
        })
        
    def get_positions(self) -> Dict[int, int]:
        """获取当前所有舵机位置"""
        self.send_command({'cmd': 'get_positions'})
        time.sleep(0.1)  # 等待响应
        return self.servo_positions.copy()
        
    def enable_torque(self, servo_id: int, enable: bool = True) -> bool:
        """开启/关闭舵机扭矩"""
        return self.send_command({
            'cmd': 'set_torque',
            'id': servo_id,
            'enable': enable
        })
        
    def set_mode(self, mode: str) -> bool:
        """设置工作模式"""
        return self.send_command({
            'cmd': 'set_mode',
            'mode': mode
        })
        
    def scan_servos(self) -> List[int]:
        """扫描舵机"""
        self.send_command({'cmd': 'scan'})
        time.sleep(2)  # 等待扫描完成
        return list(self.servo_positions.keys())
        
    def get_servo_status(self, servo_id: int) -> Optional[ServoStatus]:
        """获取舵机状态"""
        return self.servo_status.get(servo_id)


def find_esp32_port():
    """自动查找 ESP32 串口"""
    import sys
    
    if sys.platform == 'darwin':
        import glob
        ports = glob.glob('/dev/cu.usbserial-*') + glob.glob('/dev/cu.SLAB_USBtoUART')
    elif sys.platform == 'linux':
        import glob
        ports = glob.glob('/dev/ttyUSB*') + glob.glob('/dev/ttyACM*')
    elif sys.platform == 'win32':
        import serial.tools.list_ports
        ports = [p.device for p in serial.tools.list_ports.comports() 
                if 'USB' in p.description or 'UART' in p.description]
    else:
        ports = []
        
    return ports[0] if ports else None


# 示例用法
if __name__ == '__main__':
    # 自动查找端口
    port = find_esp32_port()
    if not port:
        port = input("Enter serial port: ")
        
    # 创建客户端
    client = LeRobotClient(port=port)
    
    # 连接
    if client.connect():
        try:
            # 扫描舵机
            print("Scanning servos...")
            servos = client.scan_servos()
            print(f"Found servos: {servos}")
            
            # 读取位置
            positions = client.get_positions()
            print(f"Current positions: {positions}")
            
            # 控制示例
            if servos:
                print("Moving servo 1 to center position...")
                client.set_position(1, 2047, speed=500)
                time.sleep(2)
                
                print("Moving servo 1 to min position...")
                client.set_position(1, 0, speed=1000)
                time.sleep(2)
                
                print("Moving servo 1 to max position...")
                client.set_position(1, 4095, speed=1000)
                time.sleep(2)
                
                print("Returning to center...")
                client.set_position(1, 2047, speed=500)
                
        except KeyboardInterrupt:
            print("\nInterrupted by user")
        finally:
            client.disconnect()
    else:
        print("Failed to connect")
