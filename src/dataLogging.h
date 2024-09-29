#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP_Google_Sheet_Client.h>
#include <ESPmDNS.h>
// #include <GS_SDHelper.h>
// #include <SPIFFS.h>
#include <Wire.h>
#include <webServer.h>
#include <adc.h>
#include <time.h>

#define UNIT_NO "001"

extern const char PRIVATE_KEY[] PROGMEM;

// The ID of the spreadsheet where you'll publish the data
extern const char spreadsheetId[];

// Timer variables
extern unsigned long lastTime;
extern int dataLogInterval;

// NTP server to request epoch time
extern const char* ntpServer;
extern const long gmtOffset_sec;

// Variable to save current epoch time
extern unsigned long epochTime;

extern bool logData;
extern unsigned long dataLoggingStartMillis, dataLoggingStopMillis, dataLoggingLastModifiedMillis;

void googleSheetSetup();
void googleSheetLoop(actualData read);
void tokenStatusCallback(TokenInfo info);
// void loggingStatusDisplay();