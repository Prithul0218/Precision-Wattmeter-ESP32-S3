#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>  // Changed from SPIFFS to LittleFS
#include <Wire.h>
#include <adc.h>
#include <webServer.h>
// #include <WiFiManager.h>
// Create an instance of the web server
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char *ssid = "BD LINK ATL";
const char *password = "MIRPUR@12161230";

void serverSetup() {
    // Initialize LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("An error has occurred while mounting LittleFS");
        return;
    }
    Serial.println("LittleFS mounted successfully");

    // Connect to Wi-Fi
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println(WiFi.localIP());
    Serial.println("Connected to WiFi");

    if (!MDNS.begin("esp32")) {
        Serial.println("Error setting up MDNS responder!");
        while (1) {
            delay(1000);
        }
    }
    Serial.println("mDNS responder started");

    // Start TCP (HTTP) server
    server.begin();

    // WebSocket event handler
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    // Serve static HTML file
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/index.html", "text/html");  // Changed from SPIFFS to LittleFS
    });

    // Start server
    server.begin();
    Serial.println("TCP server started");

    // Add service to MDNS-SD
    MDNS.addService("http", "tcp", 80);
}

unsigned long lastServerLoop = 0;
byte serverUpdateInterval = 5;

void serverLoop(actualData data) {
    if (millis() - lastServerLoop < serverUpdateInterval * 1000) {
        return;
    }
    lastServerLoop = millis();
    ws.cleanupClients();

    DynamicJsonDocument doc(1024);
    doc["voltage"] = readData.voltage;
    doc["current"] = readData.current;
    doc["power"] = readData.power;
    doc["energy"] = readData.energy;
    doc["time"] = elapsedTime / 1000;
    doc["peakVoltage"] = readData.peakVoltage;
    doc["peakCurrent"] = readData.peakCurrent;
    doc["peakPower"] = readData.peakPower;
    doc["minVoltage"] = readData.minVoltage;
    doc["minCurrent"] = readData.minCurrent;
    doc["minPower"] = readData.minPower;

    String jsonString;
    serializeJson(doc, jsonString);
    // Serial.println(jsonString);
    ws.textAll(jsonString);
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    Serial.println("onWsEvent()");
    if (type == WS_EVT_CONNECT) {
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->final && info->index == 0 && info->len == len) {
            if (info->opcode == WS_TEXT) {
                data[len] = 0;
                DynamicJsonDocument doc(1024);
                deserializeJson(doc, (char *)data);
                const char *type = doc["type"];

                if (strcmp(type, "calibration") == 0) {
                    calibration.voltageOffset = doc["data"]["voltageOffset"];
                    calibration.voltageGainCurrent = doc["data"]["voltageGain"];
                    calibration.currentOffset5m = doc["data"]["currentOffset"];
                    calibration.currentGain100m = doc["data"]["currentGain"];
                    // Handle calibration data
                } else if (strcmp(type, "advancedCalibration") == 0) {
                    float pg2 = doc["data"]["pg2"];
                    float pg4 = doc["data"]["pg4"];
                    float pg8 = doc["data"]["pg8"];
                    float pg16 = doc["data"]["pg16"];
                    float currentGain5 = doc["data"]["currentGain5"];
                    float currentGain100 = doc["data"]["currentGain100"];
                    // Handle advanced calibration data
                } else if (strcmp(type, "resetEnergy") == 0) {
                    readData.energy = 0;
                } else if (strcmp(type, "resetPeak") == 0) {
                    readData.peakVoltage = 0;
                    readData.peakCurrent = 0;
                    readData.peakPower = 0;
                    readData.peakChanged = true;
                } else if (strcmp(type, "resetMin") == 0) {
                    readData.minVoltage = 99;
                    readData.minCurrent = 99;
                    readData.minPower = 999;
                } else if (strcmp(type, "resetTimer") == 0) {
                    resetStopwatch();
                } else if (strcmp(type, "holdOn") == 0) {
                    hold = doc["data"]["holdOn"];
                    Serial.println("holdon: " + String(hold));
                } else if (strcmp(type, "force5mShunt") == 0) {
                    forceUse5mShunt = doc["data"]["forceUse5mShunt"];
                } else if (strcmp(type, "force100mShunt") == 0) {
                    forceUse5mShunt = false;
                } else if (strcmp(type, "setLowVoltageAlarm") == 0) {
                    alarmVoltage = doc["data"]["alarmVoltage"] ? doc["data"]["alarmVoltage"] : alarmVoltage;
                    lowVoltageAlarm = doc["data"]["alarmVoltage"] != NAN ? doc["data"]["alarmVoltage"] : lowVoltageAlarm;
                }
            }
        }
    }
}
