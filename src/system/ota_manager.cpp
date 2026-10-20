/**
 * @file ota_manager.cpp
 * @brief Implementation of Wireless Over-The-Air Web Portal and Wi-Fi Configuration.
 */

#include "ota_manager.h"

namespace System {

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>SENTINAL OTA</title>
<style>
* { box-sizing: border-box; margin: 0; padding: 0; }
body {
  background-color: #000000;
  color: #ffffff;
  font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", "Courier New", monospace;
  font-size: 13px;
  line-height: 1.5;
  padding: 32px 16px;
  display: flex;
  justify-content: center;
}
.box {
  width: 100%;
  max-width: 420px;
  border: 1px solid #222222;
  background-color: #080808;
  padding: 24px;
}
.title {
  font-size: 14px;
  font-weight: 700;
  letter-spacing: 2px;
  text-transform: uppercase;
  border-bottom: 1px solid #222222;
  padding-bottom: 12px;
  margin-bottom: 20px;
}
.sec-title {
  font-size: 11px;
  letter-spacing: 1.5px;
  text-transform: uppercase;
  color: #777777;
  margin-bottom: 12px;
}
.row {
  display: flex;
  justify-content: space-between;
  padding: 5px 0;
  border-bottom: 1px solid #141414;
  color: #999999;
}
.row-val {
  color: #ffffff;
}
.section {
  margin-bottom: 24px;
}
input[type="text"], input[type="password"] {
  width: 100%;
  background: #000000;
  border: 1px solid #262626;
  color: #ffffff;
  padding: 8px 10px;
  font-size: 12px;
  font-family: inherit;
  margin-bottom: 8px;
  outline: none;
}
input[type="text"]:focus, input[type="password"]:focus {
  border-color: #ffffff;
}
.file-btn {
  display: block;
  border: 1px dashed #333333;
  padding: 12px;
  text-align: center;
  color: #888888;
  cursor: pointer;
  margin-bottom: 12px;
  font-size: 11px;
  letter-spacing: 1px;
}
.file-btn:hover {
  border-color: #ffffff;
  color: #ffffff;
}
button {
  width: 100%;
  background: #ffffff;
  color: #000000;
  border: none;
  padding: 10px;
  font-size: 11px;
  font-weight: 700;
  letter-spacing: 1.5px;
  text-transform: uppercase;
  cursor: pointer;
  font-family: inherit;
}
button:hover {
  background: #cccccc;
}
button:disabled {
  background: #222222;
  color: #555555;
  cursor: not-allowed;
}
.bar-track {
  width: 100%;
  background: #141414;
  border: 1px solid #222222;
  height: 4px;
  margin: 12px 0 6px 0;
  display: none;
}
.bar-fill {
  width: 0%;
  height: 100%;
  background: #ffffff;
}
.msg {
  font-size: 11px;
  color: #888888;
  margin-top: 8px;
  min-height: 16px;
  word-break: break-all;
}
</style>
</head>
<body>
<div class="box">
  <div class="title">SENTINAL OTA</div>
  <div class="section">
    <div class="sec-title">STATUS</div>
    <div class="row"><span>DEVICE</span><span class="row-val">ESP32-S3</span></div>
    <div class="row"><span>FIRMWARE</span><span class="row-val">v1.0.0</span></div>
    <div class="row"><span>SOFT AP</span><span class="row-val" id="apIp">192.168.4.1</span></div>
    <div class="row"><span>WIFI STA</span><span class="row-val" id="staStatus">DISCONNECTED</span></div>
    <div class="row"><span>STA IP</span><span class="row-val" id="staIp">-</span></div>
  </div>
  <div class="section">
    <div class="sec-title">WIFI CONFIGURATION</div>
    <input type="text" id="ssid" placeholder="NETWORK SSID">
    <input type="password" id="pass" placeholder="PASSWORD">
    <button onclick="connectWiFi()">CONNECT TO WIFI</button>
    <div class="msg" id="wifiMsg"></div>
  </div>
  <div class="section">
    <div class="sec-title">FIRMWARE UPDATE</div>
    <label class="file-btn" id="fileLabel" for="fileInput">SELECT FIRMWARE (.BIN)</label>
    <input type="file" id="fileInput" accept=".bin" onchange="fileChosen(this)" style="display:none">
    <button id="uploadBtn" onclick="uploadFirmware()" disabled>UPLOAD FIRMWARE</button>
    <div class="bar-track" id="barTrack"><div class="bar-fill" id="barFill"></div></div>
    <div class="msg" id="uploadMsg"></div>
  </div>
</div>
<script>
function fileChosen(i) {
  var f = i.files[0];
  var l = document.getElementById('fileLabel');
  var b = document.getElementById('uploadBtn');
  if (f) {
    l.innerText = f.name;
    b.disabled = false;
  }
}
function uploadFirmware() {
  var i = document.getElementById('fileInput');
  if (!i.files.length) return;
  var b = document.getElementById('uploadBtn');
  var t = document.getElementById('barTrack');
  var f = document.getElementById('barFill');
  var m = document.getElementById('uploadMsg');
  b.disabled = true;
  t.style.display = 'block';
  m.innerText = 'TRANSMITTING... [ 0% ]';
  var fd = new FormData();
  fd.append('update', i.files[0]);
  var x = new XMLHttpRequest();
  x.upload.onprogress = function(e) {
    if (e.lengthComputable) {
      var p = Math.round((e.loaded / e.total) * 100);
      f.style.width = p + '%';
      m.innerText = 'TRANSMITTING... [ ' + p + '% ]';
    }
  };
  x.onload = function() {
    if (x.status === 200) {
      f.style.width = '100%';
      m.innerText = 'UPDATE SUCCESSFUL. REBOOTING...';
      setTimeout(function() { location.reload(); }, 6000);
    } else {
      m.innerText = 'UPDATE FAILED (' + x.status + ')';
      b.disabled = false;
    }
  };
  x.onerror = function() {
    m.innerText = 'CONNECTION ERROR';
    b.disabled = false;
  };
  x.open('POST', '/update', true);
  x.send(fd);
}
function connectWiFi() {
  var s = document.getElementById('ssid').value;
  var p = document.getElementById('pass').value;
  var m = document.getElementById('wifiMsg');
  if (!s) { m.innerText = 'SSID REQUIRED'; return; }
  m.innerText = 'CONNECTING...';
  var fd = new FormData();
  fd.append('ssid', s);
  fd.append('pass', p);
  fetch('/wifi', { method: 'POST', body: fd })
    .then(r => r.json())
    .then(d => {
      m.innerText = 'COMMAND SENT. POLLING STATUS...';
      pollStatus();
    })
    .catch(() => { m.innerText = 'REQUEST FAILED'; });
}
function pollStatus() {
  fetch('/status')
    .then(r => r.json())
    .then(d => {
      document.getElementById('apIp').innerText = d.ap_ip || '192.168.4.1';
      document.getElementById('staStatus').innerText = d.wifi_connected ? 'CONNECTED (' + d.ssid + ')' : 'DISCONNECTED';
      document.getElementById('staIp').innerText = d.wifi_connected ? d.ip : '-';
    })
    .catch(() => {});
}
setInterval(pollStatus, 3000);
pollStatus();
</script>
</body>
</html>
)rawliteral";

OTAManager::OTAManager()
    : _server(Config::Settings::OTA_PORT),
      _isActive(false),
      _apStarted(false) {}

void OTAManager::init() {
    loadSavedWiFi();
}

void OTAManager::loadSavedWiFi() {
    _prefs.begin("sentinal", true);
    _savedSSID = _prefs.getString("wifi_ssid", "");
    _savedPass = _prefs.getString("wifi_pass", "");
    _prefs.end();
}

void OTAManager::saveWiFi(const String& ssid, const String& pass) {
    _prefs.begin("sentinal", false);
    _prefs.putString("wifi_ssid", ssid);
    _prefs.putString("wifi_pass", pass);
    _prefs.end();
    _savedSSID = ssid;
    _savedPass = pass;
}

void OTAManager::setupRoutes() {
    // 1. Root Portal
    _server.on("/", HTTP_GET, [this]() {
        handleRoot();
    });

    // 2. Status Poller (JSON)
    _server.on("/status", HTTP_GET, [this]() {
        handleStatus();
    });

    // 3. Wi-Fi Connect Trigger
    _server.on("/wifi", HTTP_POST, [this]() {
        handleWiFiConnect();
    });

    // 4. Restart Trigger
    _server.on("/restart", HTTP_POST, [this]() {
        handleRestart();
    });

    // 5. Binary Firmware Upload
    _server.on("/update", HTTP_POST,
        [this]() {
            bool success = !Update.hasError();
            _server.sendHeader("Connection", "close");
            _server.send(success ? 200 : 500, "text/plain", success ? "OK" : "FAIL");
            if (success) {
                delay(500);
                ESP.restart();
            }
        },
        [this]() {
            HTTPUpload& upload = _server.upload();
            if (upload.status == UPLOAD_FILE_START) {
                Serial.printf("\n[OTA] Receiving firmware upload: %s\n", upload.filename.c_str());
                if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
                    Update.printError(Serial);
                }
            } else if (upload.status == UPLOAD_FILE_WRITE) {
                if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                    Update.printError(Serial);
                }
            } else if (upload.status == UPLOAD_FILE_END) {
                if (Update.end(true)) {
                    Serial.printf("[OTA] Firmware write complete: %u bytes. Rebooting device...\n", upload.totalSize);
                } else {
                    Update.printError(Serial);
                }
            } else if (upload.status == UPLOAD_FILE_ABORTED) {
                Update.end();
                Serial.println("[OTA] Firmware upload aborted by client.");
            }
        }
    );
}

void OTAManager::begin() {
    if (_isActive) return;

    Serial.println("\n[OTA] Initializing Wireless OTA Portal...");

    // 1. Set Wi-Fi to Dual AP + Station mode
    WiFi.mode(WIFI_AP_STA);

    // 2. Start SoftAP
    _apStarted = WiFi.softAP(Config::Settings::OTA_AP_SSID, Config::Settings::OTA_AP_PASS);
    Serial.printf("[OTA] SoftAP '%s' started. Access IP: %s\n",
                  Config::Settings::OTA_AP_SSID,
                  WiFi.softAPIP().toString().c_str());

    // 3. If saved station credentials exist, begin connection
    if (_savedSSID.length() > 0) {
        Serial.printf("[OTA] Connecting to saved Wi-Fi '%s'...\n", _savedSSID.c_str());
        WiFi.begin(_savedSSID.c_str(), _savedPass.c_str());
    }

    // 4. Initialize mDNS (sentinal.local)
    if (MDNS.begin(Config::Settings::OTA_MDNS_HOST)) {
        MDNS.addService("http", "tcp", Config::Settings::OTA_PORT);
        Serial.printf("[OTA] mDNS responder started: http://%s.local\n", Config::Settings::OTA_MDNS_HOST);
    }

    // 5. Setup HTTP routes and start server
    setupRoutes();
    _server.begin();
    _isActive = true;

    Serial.println("[OTA] Portal ready. Connect to Wi-Fi 'Sentinal-OTA' and navigate to http://192.168.4.1\n");
}

void OTAManager::stop() {
    if (!_isActive) return;

    Serial.println("[OTA] Stopping OTA Server and disabling Wi-Fi radio...");
    _server.stop();
    MDNS.end();
    WiFi.disconnect(true);
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    _isActive = false;
    _apStarted = false;
    Serial.println("[OTA] Wi-Fi radio powered off.\n");
}

void OTAManager::handleClient() {
    if (_isActive) {
        _server.handleClient();
    }
}

void OTAManager::handleRoot() {
    _server.send(200, "text/html", INDEX_HTML);
}

void OTAManager::handleStatus() {
    bool staConnected = (WiFi.status() == WL_CONNECTED);
    String json = "{";
    json += "\"device\":\"ESP32-S3\",";
    json += "\"firmware\":\"" + String(Config::Settings::FIRMWARE_VERSION) + "\",";
    json += "\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\",";
    json += "\"wifi_connected\":" + String(staConnected ? "true" : "false") + ",";
    json += "\"ssid\":\"" + (staConnected ? WiFi.SSID() : "") + "\",";
    json += "\"ip\":\"" + (staConnected ? WiFi.localIP().toString() : "") + "\"";
    json += "}";
    _server.send(200, "application/json", json);
}

void OTAManager::handleWiFiConnect() {
    String ssid = _server.arg("ssid");
    String pass = _server.arg("pass");

    if (ssid.length() > 0) {
        Serial.printf("[OTA] Wi-Fi connect request received for: %s\n", ssid.c_str());
        saveWiFi(ssid, pass);
        WiFi.begin(ssid.c_str(), pass.c_str());
        _server.send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
        _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"SSID required\"}");
    }
}

void OTAManager::handleRestart() {
    _server.send(200, "text/plain", "RESTARTING");
    delay(500);
    ESP.restart();
}

String OTAManager::getStationIP() const {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    }
    return String("-");
}

String OTAManager::getAPIP() const {
    return WiFi.softAPIP().toString();
}

String OTAManager::getStationSSID() const {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.SSID();
    }
    return String("");
}

} // namespace System
