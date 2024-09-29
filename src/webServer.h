#pragma once
#include<Arduino.h>
#include<Wire.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h> // Changed from SPIFFS to LittleFS
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
// #include <FS.h>
#include <ESPmDNS.h>

extern const char* ssid;
extern const char* password;

// Create an instance of the web server
extern AsyncWebServer server;
extern byte serverUpdateInterval;

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void serverSetup();
void serverLoop(actualData data);

