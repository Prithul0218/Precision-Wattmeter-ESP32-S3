#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <adc.h>
#include <oled.h>

// Battery icons (in XBM format for U8g2)
#define batteryWidth 10
#define batteryHeight 6
static const unsigned char battery0Bits[] U8X8_PROGMEM = {
    0xFF, 0x01, 0x01, 0x01, 0x01, 0x03, 0x01, 0x03, 0x01, 0x01, 0xFF, 0x01};
static const unsigned char battery1Bits[] U8X8_PROGMEM = {
    0xFF, 0x01, 0x01, 0x01, 0x05, 0x03, 0x05, 0x03, 0x01, 0x01, 0xFF, 0x01};
static const unsigned char battery2Bits[] U8X8_PROGMEM = {
    0xFF, 0x01, 0x01, 0x01, 0x15, 0x03, 0x15, 0x03, 0x01, 0x01, 0xFF, 0x01};
static const unsigned char battery3Bits[] U8X8_PROGMEM = {
    0xFF, 0x01, 0x01, 0x01, 0x55, 0x03, 0x55, 0x03, 0x01, 0x01, 0xFF, 0x01};

// Array of battery icons
const unsigned char* batteryIcon[4] = {
    battery0Bits,
    battery1Bits,
    battery2Bits,
    battery3Bits};

// Currently displayed icon (should be determined dynamically based on real battery status)
byte batteryLevel = 3;

// U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2(U8G2_R2, 2, 1, /* reset=*/U8X8_PIN_NONE);

// LCD and menu---------------------------------------------------------
byte volatile screen = 0;
byte displayContrast = 9;

void drawBatteryStatus() {
    // Draw battery icon in top right corner of the screen
    u8g2.drawXBMP((u8g2.getDisplayWidth() - batteryWidth) - 1, 0, batteryWidth, batteryHeight, batteryIcon[batteryLevel]);
}

void initializeDispaly() {
    Wire.setClock(1000000L);
    u8g2.begin();
    u8g2.setContrast(21 + (displayContrast * 26));
    // u8g2.setDisplayRotation(1);
};

unsigned long previousDisplayUpdate = 0;
void displayMainScreen(uint8_t screen, actualData data) {
    // Serial.println("Previous Display Update: " + String(micros() - previousDisplayUpdate));
    // previousDisplayUpdate = micros();
    
    // if()
    // float power = readData.voltage * readData.current;
    // uint16_t timeH = (millis() - starTimeMs) / 3600000;
    // float energy = voltage * current * (float)timeH;
    // u8g2.setFont(u8g2_font_helvB10_tf);
    unsigned long seconds = (elapsedTime) / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    // Remaining seconds after converting to minutes
    seconds %= 60;
    // Remaining minutes after converting to hours
    minutes %= 60;
    switch (screen) {
        case 0:
            u8g2.clearBuffer();  // clear the internal memory
            u8g2.setFont(u8g2_font_helvB12_tf);
            // u8g2.setFont(u8g2_font_ncenB10_tr);	// choose a suitable font
            u8g2.setCursor(0, 0);
            u8g2.print(data.voltage, data.voltage > 10 ? 4 : 5);
            u8g2.setFont(u8g2_font_helvR08_tf);
            u8g2.setCursor(58, 3);
            u8g2.print("V");

            u8g2.setFont(u8g2_font_helvB12_tf);
            u8g2.setCursor(0, 13);
            u8g2.print(data.current, data.current > 10 ? 4 : 5);
            u8g2.setFont(u8g2_font_helvR08_tf);
            u8g2.setCursor(58, 16);
            u8g2.print("A");

            u8g2.setFont(u8g2_font_helvB12_tf);
            u8g2.setCursor(0, 28);
            u8g2.print(data.power, data.power > 100 ? 3 : data.power > 10 ? 4
                                                                          : 5);
            u8g2.setFont(u8g2_font_helvR08_tf);
            u8g2.setCursor(58, 31);
            u8g2.print("W");

            u8g2.setFont(u8g2_font_helvR10_tf);

            u8g2.setCursor(0, 48);
            if (hours < 10)
                u8g2.print('0');
            u8g2.print(hours);
            u8g2.print(":");
            if (minutes < 10)
                u8g2.print('0');
            u8g2.print(minutes);
            u8g2.print(":");
            if (seconds < 10)
                u8g2.print('0');
            u8g2.print(seconds);

            u8g2.setCursor(73, 0);
            u8g2.print(data.mAh, data.mAh > 1000 ? 0 : data.mAh > 100 ? 1
                                                   : data.mAh > 10    ? 2
                                                                      : 3);
            u8g2.setFont(u8g2_font_helvR08_tf);
            u8g2.setCursor(108, 2);
            u8g2.print("mAh");

            u8g2.setFont(u8g2_font_helvR10_tf);
            u8g2.setCursor(73, 14);
            u8g2.print(data.energy, data.energy > 100 ? 1 : data.energy > 10 ? 2
                                                                             : 3);
            u8g2.setFont(u8g2_font_helvR08_tf);
            u8g2.setCursor(108, 16);
            u8g2.print("Wh");

            // u8g2.setCursor(69, 36);
            // u8g2.print((millis() - starTimeMs) / 60000);
            // u8g2.print("min");

            if (data.peakChanged == true) {
                const byte FONT_HEIGHT_HERE = 8;
                const byte START_PIXEL_HERE_X = 72;
                const byte START_PIXEL_HERE_Y = 32;
                u8g2.setFont(u8g2_font_helvR08_te);

                u8g2.setCursor(START_PIXEL_HERE_X + FONT_HEIGHT_HERE + 6, START_PIXEL_HERE_Y);
                u8g2.print(data.peakVoltage, data.peakVoltage > 10 ? 3 : 4);
                u8g2.print("V");

                u8g2.setCursor(START_PIXEL_HERE_X + FONT_HEIGHT_HERE + 6, START_PIXEL_HERE_Y + FONT_HEIGHT_HERE + 3);
                u8g2.print(data.peakCurrent, data.peakCurrent > 10 ? 3 : 4);
                u8g2.print("A");

                u8g2.setCursor(START_PIXEL_HERE_X + FONT_HEIGHT_HERE + 6, START_PIXEL_HERE_Y + FONT_HEIGHT_HERE + 3 + FONT_HEIGHT_HERE + 3);
                u8g2.print(data.peakPower, data.peakPower > 100 ? 2 : data.peakPower > 10 ? 3
                                                                                          : 4);
                u8g2.print("W");

                u8g2.setFontDirection(3);
                u8g2.setCursor(START_PIXEL_HERE_X, START_PIXEL_HERE_Y + 25);
                u8g2.print("Max");
                u8g2.setFontDirection(0);
                u8g2.drawVLine(START_PIXEL_HERE_X + FONT_HEIGHT_HERE + 3, START_PIXEL_HERE_Y, (FONT_HEIGHT_HERE * 3) + 3 * 3);

                data.peakChanged = false;
            }

            // u8g2.drawStr(0, 24, "19.692W 2.999Wh");  // write something to the internal memory
            u8g2.sendBuffer();  // transfer internal memory to the display

            break;

        default:
            break;
    }
};

void displayLoggingStatus(bool logData, unsigned long statusChangeMillis) {
    // if()
    // float power = readData.voltage * readData.current;
    // uint16_t timeH = (millis() - starTimeMs) / 3600000;
    // float energy = voltage * current * (float)timeH;
    // u8g2.setFont(u8g2_font_helvB10_tf);

    u8g2.clearBuffer();  // clear the internal memory
    u8g2.setFont(u8g2_font_helvR08_tf);
    // u8g2.setFont(u8g2_font_ncenB10_tr);	// choose a suitable font
    if (statusChangeMillis == 0) {
        u8g2.setCursor(0, 10);
        u8g2.print("Logging Status:");
        u8g2.setCursor(0, 20);
        u8g2.print("Not Started");
        u8g2.sendBuffer();  // transfer internal memory to the display
        return;
    }
    if (logData == true) {
        u8g2.setCursor(0, 10);
        u8g2.print("Logging Started:");
        u8g2.setCursor(0, 20);
        u8g2.print(statusChangeMillis / 1000);
    } else {
        u8g2.setCursor(0, 10);
        u8g2.print("Logging Stopped:");
        u8g2.setCursor(0, 20);
        u8g2.print(statusChangeMillis / 1000);
    }
    u8g2.sendBuffer();  // transfer internal memory to the display
};