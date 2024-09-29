#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <adc.h>

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;  // Declare the lcd object

// LCD and menu---------------------------------------------------------
extern byte volatile screen;
extern byte displayContrast;

void initializeDispaly();
void displayMainScreen(uint8_t screen, actualData data);
void displayLoggingStatus(bool logData, unsigned long statusChangeMillis) ;
void drawBatteryStatus();

// #define buzzerPin 13
// extern bool timerRunning;
// extern bool beeper;

// extern unsigned long  lastButtonPress;
// extern bool  menu;
// extern bool  submenu;
// extern byte  currentMenuOption;
// extern byte  currentSubmenuOption;
// extern byte  currentSubsubmenuOption;
// extern byte  totalOptions;
// extern bool submenuSelecteed;
// extern bool subsubmenuSeleted;
// extern bool autoBacklight;
// extern byte man[8];
// extern bool  alternatingOption;

// extern unsigned long lastBacklightCheck;
// extern byte backLightCount;
// #define LDRPin 13;
// extern byte date;
// extern byte lastDate;

// void printLocalTime();
// void setContrast();
// void printPresentDuration(unsigned long milliseconds, byte row, byte col);
//---------------------------------------------------------------------
