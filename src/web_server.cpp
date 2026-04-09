// web_server.cpp - Web 服务器实现
#include "web_server.h"

WebServerManager::WebServerManager() {
    server = nullptr;
    initialized = false;
}

WebServerManager::~WebServerManager() {
    end();
}

bool WebServerManager::begin() {
    if (initialized) return true;
    
    // 确保 WiFi AP 已启动
    if (!WiFi.softAPgetStationNum() && WiFi.getMode() != WIFI_AP) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
    }
    
    // 创建服务器
    server = new WebServer(WEB_SERVER_PORT);
    
    // 设置路由
    server->on("/", HTTP_GET, [this]() { handleRoot(); });
    server->on("/api/servos", HTTP_GET, [this]() { handleServoList(); });
    server->on("/api/servo/control", HTTP_POST, [this]() { handleServoControl(); });
    server->on("/api/servo/status", HTTP_GET, [this]() { handleServoStatus(); });
    server->on("/api/mode", HTTP_POST, [this]() { handleSetMode(); });
    server->on("/api/scan", HTTP_POST, [this]() { handleScan(); });
    server->onNotFound([this]() { handleNotFound(); });
    
    // 启动服务器
    server->begin();
    
    initialized = true;
    DEBUG_PRINTF("Web server started on port %d\n", WEB_SERVER_PORT);
    DEBUG_PRINTF("Access: http://%s/\n", WiFi.softAPIP().toString().c_str());
    
    return true;
}

void WebServerManager::end() {
    if (server) {
        server->close();
        delete server;
        server = nullptr;
    }
    initialized = false;
}

void WebServerManager::handleClient() {
    if (server) {
        server->handleClient();
    }
}

void WebServerManager::processCommands(ServoDriver& servoDriver) {
    // 在 handleClient 中处理
}

void WebServerManager::handleRoot() {
    server->send(200, "text/html", getIndexHTML());
}

void WebServerManager::handleServoList() {
    // TODO: 返回舵机列表 JSON
    String json = "{\"servos\":[],\"count\":0}";
    server->send(200, "application/json", json);
}

void WebServerManager::handleServoControl() {
    // TODO: 解析 POST 数据并控制舵机
    if (server->hasArg("plain")) {
        String body = server->arg("plain");
        
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, body);
        
        if (!error) {
            int id = doc["id"] | 0;
            int position = doc["position"] | 2048;
            int speed = doc["speed"] | 0;
            
            // 这里应该调用 servoDriver 的方法
            // servoDriver.setPosition(id, position, speed);
            
            String response = "{\"success\":true,\"id\":" + String(id) + 
                            ",\"position\":" + String(position) + "}";
            server->send(200, "application/json", response);
            return;
        }
    }
    
    server->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid request\"}");
}

void WebServerManager::handleServoStatus() {
    // TODO: 返回舵机状态 JSON
    String json = "{\"status\":\"ok\",\"servos\":[]}";
    server->send(200, "application/json", json);
}

void WebServerManager::handleSetMode() {
    // TODO: 设置工作模式
    server->send(200, "application/json", "{\"success\":true}");
}

void WebServerManager::handleScan() {
    // TODO: 扫描舵机
    server->send(200, "application/json", "{\"success\":true,\"count\":0}");
}

void WebServerManager::handleNotFound() {
    server->send(404, "text/plain", "Not Found");
}

String WebServerManager::getIndexHTML() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>LeRobot ESP32 Controller</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: #1a1a2e;
            color: #eee;
            padding: 20px;
            max-width: 600px;
            margin: 0 auto;
        }
        h1 { text-align: center; color: #00d4ff; margin-bottom: 20px; }
        .card {
            background: #16213e;
            border-radius: 12px;
            padding: 20px;
            margin-bottom: 20px;
        }
        .card h2 {
            font-size: 16px;
            margin-bottom: 15px;
            color: #00d4ff;
        }
        .status-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 10px;
        }
        .status-item {
            background: #0f3460;
            padding: 10px;
            border-radius: 8px;
        }
        .status-item label {
            font-size: 12px;
            color: #888;
            display: block;
        }
        .status-item span {
            font-size: 18px;
            font-weight: bold;
        }
        .btn {
            background: #e94560;
            color: white;
            border: none;
            padding: 12px 24px;
            border-radius: 8px;
            cursor: pointer;
            font-size: 16px;
            margin: 5px;
            transition: all 0.3s;
        }
        .btn:hover { background: #ff6b6b; }
        .btn.secondary { background: #0f3460; }
        .btn.secondary:hover { background: #1a5490; }
        .slider-container {
            margin: 15px 0;
        }
        .slider-container label {
            display: block;
            margin-bottom: 5px;
            font-size: 14px;
        }
        .slider {
            width: 100%;
            height: 40px;
            -webkit-appearance: none;
            background: #0f3460;
            border-radius: 20px;
            outline: none;
        }
        .slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 40px;
            height: 40px;
            background: #00d4ff;
            border-radius: 50%;
            cursor: pointer;
        }
        .value-display {
            text-align: center;
            font-size: 24px;
            font-weight: bold;
            color: #00d4ff;
            margin: 10px 0;
        }
        .servo-id {
            display: flex;
            justify-content: center;
            gap: 10px;
            margin: 15px 0;
        }
        .servo-id button {
            background: #0f3460;
            border: 2px solid #00d4ff;
            color: #00d4ff;
            padding: 10px 15px;
            border-radius: 8px;
            cursor: pointer;
            font-size: 14px;
        }
        .servo-id button.active {
            background: #00d4ff;
            color: #1a1a2e;
        }
        #log {
            background: #0a0a1a;
            border-radius: 8px;
            padding: 10px;
            height: 150px;
            overflow-y: auto;
            font-family: monospace;
            font-size: 12px;
            color: #0f0;
        }
    </style>
</head>
<body>
    <h1>LeRobot ESP32</h1>
    
    <div class="card">
        <h2>Device Status</h2>
        <div class="status-grid">
            <div class="status-item">
                <label>Mode</label>
                <span id="mode">Follower</span>
            </div>
            <div class="status-item">
                <label>Servos</label>
                <span id="servoCount">0</span>
            </div>
            <div class="status-item">
                <label>WiFi</label>
                <span id="wifi">Connected</span>
            </div>
            <div class="status-item">
                <label>Status</label>
                <span id="status">OK</span>
            </div>
        </div>
    </div>
    
    <div class="card">
        <h2>Servo Control</h2>
        <div class="servo-id">
            <button onclick="selectServo(1)" id="btn1" class="active">ID 1</button>
            <button onclick="selectServo(2)" id="btn2">ID 2</button>
            <button onclick="selectServo(3)" id="btn3">ID 3</button>
            <button onclick="selectServo(4)" id="btn4">ID 4</button>
            <button onclick="selectServo(5)" id="btn5">ID 5</button>
            <button onclick="selectServo(6)" id="btn6">ID 6</button>
        </div>
        
        <div class="value-display">
            Position: <span id="posValue">2047</span>
        </div>
        
        <div class="slider-container">
            <input type="range" min="0" max="4095" value="2047" class="slider" id="positionSlider">
        </div>
        
        <div style="text-align: center;">
            <button class="btn" onclick="setPosition()">Set Position</button>
            <button class="btn secondary" onclick="centerPosition()">Center</button>
            <button class="btn secondary" onclick="releaseTorque()">Release</button>
        </div>
    </div>
    
    <div class="card">
        <h2>Actions</h2>
        <div style="text-align: center;">
            <button class="btn" onclick="scanServos()">Scan Servos</button>
            <button class="btn secondary" onclick="switchMode()">Switch Mode</button>
            <button class="btn secondary" onclick="clearLog()">Clear Log</button>
        </div>
    </div>
    
    <div class="card">
        <h2>Log</h2>
        <div id="log"></div>
    </div>
    
    <script>
        let currentServoId = 1;
        let position = 2047;
        
        const slider = document.getElementById('positionSlider');
        const posValue = document.getElementById('posValue');
        const log = document.getElementById('log');
        
        slider.addEventListener('input', function() {
            position = this.value;
            posValue.textContent = position;
        });
        
        function selectServo(id) {
            currentServoId = id;
            document.querySelectorAll('.servo-id button').forEach(btn => {
                btn.classList.remove('active');
            });
            document.getElementById('btn' + id).classList.add('active');
            logMessage('Selected servo ID: ' + id);
        }
        
        function setPosition() {
            fetch('/api/servo/control', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    id: currentServoId,
                    position: parseInt(position),
                    speed: 500
                })
            })
            .then(r => r.json())
            .then(data => {
                logMessage('Set servo ' + currentServoId + ' to position ' + position);
            })
            .catch(e => logMessage('Error: ' + e));
        }
        
        function centerPosition() {
            position = 2047;
            slider.value = position;
            posValue.textContent = position;
            setPosition();
        }
        
        function releaseTorque() {
            logMessage('Torque released for servo ' + currentServoId);
        }
        
        function scanServos() {
            fetch('/api/scan', { method: 'POST' })
            .then(r => r.json())
            .then(data => {
                logMessage('Scan complete. Found ' + (data.count || 0) + ' servos');
            })
            .catch(e => logMessage('Error: ' + e));
        }
        
        function switchMode() {
            logMessage('Mode switched (long press BOOT button on device)');
        }
        
        function clearLog() {
            log.innerHTML = '';
        }
        
        function logMessage(msg) {
            const time = new Date().toLocaleTimeString();
            log.innerHTML += '[' + time + '] ' + msg + '<br>';
            log.scrollTop = log.scrollHeight;
        }
        
        // Update status periodically
        setInterval(() => {
            fetch('/api/servo/status')
            .then(r => r.json())
            .then(data => {
                document.getElementById('servoCount').textContent = data.count || 0;
            })
            .catch(() => {});
        }, 1000);
        
        logMessage('Web interface loaded');
    </script>
</body>
</html>
)rawliteral";
    
    return html;
}

String WebServerManager::getServoControlJS() {
    return "";  // JavaScript 已内嵌在 HTML 中
}