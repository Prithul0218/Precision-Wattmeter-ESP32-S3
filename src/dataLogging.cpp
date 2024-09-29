#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
// #include <SPIFFS.h>
#include <Wire.h>
#include <adc.h>
#include <dataLogging.h>
#include <webServer.h>
#include <ESP_Google_Sheet_Client.h>

// Google Project ID
#define PROJECT_ID "precision-wattmeter-logging"

// Service Account's client email
#define CLIENT_EMAIL "precision-wattmeter-dataloggin@precision-wattmeter-logging.iam.gserviceaccount.com"

// Service Account's private key
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----\nMIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCnk9FSDCWvFsQK\nEakXRl6ltZFE1oU8LpJiBEW3H7IiHnkY+5GZz6rl7qsGs0NgtoIUtavJfo7CJee9\ndrzysEV5OtAqGVVhIR0AcgaDhfmIdORvHnrfUlBB05JeQRdWnm0MAH7HYhgKuCg0\nH/MYFIOho3c1EEo7KCjlWHTWb8JAwJJX4NwY5rXstNfy0TTRgyAOnnqj+vKoBXQl\nmDBLUWUPhR+9QCnSe16ApGKYkGsmZu6UV/6tDok97ocd3BBWIR69e6ysp5UASozP\nEPe7zFgGSEmbGKpCL3PZnPqI2OzqQcez7/KZUFtv6v3HS+9f4CWNf4ZS9Fe9GWIB\nfCZhjEMHAgMBAAECggEAKgOOdGKlRAuSJPAek75J1oP6Iuchyl/mxXpKvEnPEGzR\ncakI7SY6s6Et8eUZ3HKFlImjw0XepuB4BHFzl+kH0ggjzppBZLO3oLf12B3L3SFp\ncE6VAc6SXp6b0NPC9S2NK3goUPbQDkfzlHX6LaxfADSKm5w08DX2YuHVkTm92xvE\nx/PaULD9oXj/Jngm5/EKj7HWwdyaAUyjrhbmzgUkEZNw9gZsJGJu3XfBAVfb9h/8\nJ82f3xGNWnW9WGH6bkVXJuW3YoJ4pNIoDo585yT2AhyhkkAmCV1Tdsq/VS0PzzFN\nL2NczwMJYYnpbMt1BthDMvjg/N1JNtEDemnaZWeG4QKBgQDcIijlQ5PQogHPZGPK\nT1e60O1EK2G3UW9mbhdVVB0zsuL7yAP4rbnFO5MXOK8Ci/zaKruL6j1W62dmFNtQ\naGpEdCZ2hhEqgdiowAk2sAF6eXzequiUZCudMqfMjEDQ+LwldNwD9boNlPol9s8x\nmODfErQLq8x/S7oRKqizWsWENwKBgQDC4YdBiEo+W3A7H224yFAYR0SUcMmIZmPe\npyaJty9sglRq40fX6w2sZ2mYosDXTS8Cq3zzMboNoWzRIzA5zzYrGQk01qWyqEf3\nVgVw6B2cLIOQ+LruwK/s9ho2Ss0ZFpn1zvQVPb23VyxwMcHHym+X0h8Jc1DSCaqQ\nZnCoVphvsQKBgQCChHps3HFirPuXxHTqObrRWBilZ3dLYqxDNhj9jZ62zWSJViEM\nq6xOhbEDfqc9QlTL4bRLE7oEtBQdUVZrSU1gguFfOsQoyM31185er2JhBHEF8J1v\nhijZznPw8mNnw2KCbtQYQPRsIx1hrwFP7c2+VW+Mz/KysuqCTSGzHamoZwKBgHiA\nk+4FpELir4cfa+0yj7QzfUh8ZWGeTmrC2KWXTA/Alwpw91+fzJiqaTUkjjGTXFo+\nR8jpGq6K/opjQ9K7Ojd0B6lQgKz0OWvLGRozrPuA2umto4k2RJI1qwefQSseOp0w\nFuV7g+/2S1gkrDoqBs7N8rsZPwRhTcJ0VX0B7shBAoGAR18f2VsuTMcMNNyMLUCA\ntSWHUlvOrkDmJxlmZfmnJwwPAyOeF63ol0Ta8g5hg+CVNerb/KWls+j4TlfJIhii\niEvyWqAH6T4UNqf47foVQwC7WLkwWrj6iHCLhdc6UFcnxhjUckGJZkJDK0yOwdcR\n7g13Dt+lvkmM3wkwiVs/xC0=\n-----END PRIVATE KEY-----\n";

// The ID of the spreadsheet where you'll publish the data
const char spreadsheetId[] = "1-T-O5WPcCYVou4zj33XJ0Td_m33teLgL0Q6Ca7ehhU0";

// Timer variables
unsigned long lastTime = 0;
int dataLogInterval = 15;

// NTP server to request epoch time
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 6 * 3600;

// Variable to save current epoch time
unsigned long epochTime;

bool logData = false;
unsigned long dataLoggingStartMillis, dataLoggingStopMillis, dataLoggingLastModifiedMillis;

// Function that gets current epoch time
unsigned long getTime() {
    time_t now;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return (0);
    }
    time(&now);
    Serial.println("Time synchronized");
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    return now;
}

void googleSheetSetup() {
    // Configure time
    configTime(gmtOffset_sec, 0, ntpServer);

    // Now you can set the time in your library
    GSheet.setSystemTime(getTime());
    
    GSheet.printf("ESP Google Sheet Client v%s\n\n", ESP_GOOGLE_SHEET_CLIENT_VERSION);

    // Set the callback for Google API access token generation status (for debug only)
    GSheet.setTokenCallback(tokenStatusCallback);

    // Set the seconds to refresh the auth token before expire (60 to 3540, default is 300 seconds)
    GSheet.setPrerefreshSeconds(10 * 60);

    // Begin the access token generation for Google API authentication
    GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);
}
void googleSheetLoop(actualData data) {
    if(!logData)
        return;
    
    // Call ready() repeatedly in loop for authentication checking and processing
    bool ready = GSheet.ready();

    if (ready && millis() - lastTime > dataLogInterval*1000) {
        lastTime = millis();

        FirebaseJson response;

        Serial.println("\nAppend spreadsheet values...");
        Serial.println("----------------------------");

        FirebaseJson valueRange;

        // Get timestamp
        epochTime = getTime() + gmtOffset_sec;
        String timestamp = "=EPOCHTODATE(" + String(epochTime) + ")";
        // sprintf(timestamp, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        valueRange.add("majorDimension", "COLUMNS");
        valueRange.set("values/[0]/[0]", UNIT_NO);
        valueRange.set("values/[1]/[0]", timestamp);
        valueRange.set("values/[2]/[0]", readData.voltage);
        valueRange.set("values/[3]/[0]", readData.current);
        valueRange.set("values/[4]/[0]", readData.power);
        valueRange.set("values/[5]/[0]", readData.energy);
        valueRange.set("values/[6]/[0]", readData.mAh);

        // For Google Sheet API ref doc, go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/append
        // Append values to the spreadsheet
        bool success = GSheet.values.append(&response /* returned response */, spreadsheetId /* spreadsheet Id to append */, "Sheet1!A1" /* range to append */, &valueRange /* data range to append */);
        if (success) {
            response.toString(Serial, true);
            valueRange.clear();
        } else {
            Serial.println(GSheet.errorReason());
        }
        Serial.println();
        // Serial.println(ESP.getFreeHeap());
    }
}

void tokenStatusCallback(TokenInfo info) {
    if (info.status == token_status_error) {
        GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
        GSheet.printf("Token error: %s\n", GSheet.getTokenError(info).c_str());
        
        GSheet.setSystemTime(getTime());
        Serial.println("Time set: " + String(getTime()));
    } else {
        GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
    }
}

// void loggingStatusDisplay(){

// }