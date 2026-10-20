/**
 * @file ota_manager.h
 * @brief Wireless Over-The-Air (OTA) Web Portal Manager for ESP32-S3 Sentinal.
 *
 * Hosts a standalone HTTP web portal on port 80 for Wi-Fi station configuration
 * and wireless firmware (.bin) flashing. Styled in a strictly minimalist black and white theme.
 */

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include "../config/settings.h"

namespace System {

class OTAManager {
public:
    OTAManager();

    /**
     * @brief Pre-load saved Wi-Fi preferences into memory.
     */
    void init();

    /**
     * @brief Start SoftAP (Sentinal-OTA), launch HTTP web portal, and begin station connection.
     */
    void begin();

    /**
     * @brief Gracefully terminate web server and shut down Wi-Fi radio to conserve power and avoid BLE coex.
     */
    void stop();

    /**
     * @brief Loop processor for HTTP web client requests and OTA chunking.
     */
    void handleClient();

    bool isActive() const { return _isActive; }
    bool isStationConnected() const { return (WiFi.status() == WL_CONNECTED); }

    String getStationIP() const;
    String getAPIP() const;
    String getStationSSID() const;

private:
    WebServer   _server;
    Preferences _prefs;
    bool        _isActive;
    bool        _apStarted;
    String      _savedSSID;
    String      _savedPass;

    void setupRoutes();
    void handleRoot();
    void handleStatus();
    void handleWiFiConnect();
    void handleRestart();
    void loadSavedWiFi();
    void saveWiFi(const String& ssid, const String& pass);
};

} // namespace System
