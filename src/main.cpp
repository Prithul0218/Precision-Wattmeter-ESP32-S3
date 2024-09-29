#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP_Google_Sheet_Client.h>
#include <GEM_u8g2.h>
#include <KeyDetector.h>
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>
#include <adc.h>
#include <button.h>
#include <dataLogging.h>
#include <oled.h>
#include <splash.h>
#include <webServer.h>
#define RANDOMIZE_DATA
#define valueEditPrecision 3
// #include "ESP32_New_TimerInterrupt.h"

// holdData hold_data;
bool justEnteredEditMode = false;
volatile unsigned long previousADCReadMicros = 0;
volatile unsigned long previousCallMillis = 0;
TaskHandle_t task1;
static byte energyCalculationCount = 0;
const float energyCalculationInterval = 15;  // seconds

void loggingStartedMessage();
void backgroundTask(void* parameter) {
    for (;;) {
        // Serial.println("Previous call took: " + String(micros() - previousCallMillis));
        // previousCallMillis = micros();

        // constantly monitor current and if it is over 2A, switch to 5mR shunt
        // A.setMUX(DIFF_4_5);
        // A.setPGA(PGA_32);  // 156.25mV or 31.25A
        // // writeCalRegisters(PGA32Cal);
        // readData.current = A.convertToVoltage(A.readSingle());
        // readData.current *= calibration.currentGain5m;
        // if (readData.current > 2.0) digitalWrite(MOSFET_PIN, LOW);
        // if (readData.current < 0.001 && readData.current > -1) readData.current = 0.0;

        // Read ADC every second and do energy calculation every 15
        unsigned long timeTaken = micros() - previousADCReadMicros;
        if (timeTaken >= 1000000 && ADCInitialized == true) {
            previousADCReadMicros = micros();
            if (!ADCPaused) {  // we need to pause ADC when doing a self calibration
                if (timeTaken > 1001000) {
                    Serial.println("ADC took too long: " + String(timeTaken));
                    Serial.println("Please isncrease data rate!");
                    // A.setDRATE(DRATE_50SPS);
                    // delay(1000);
                } else {
                    // u8g2.setCursor(120,0);
                    // u8g2.print("*");\
                    // Serial.println("reading ADC: " + String(timeTaken));

                    readADC();
                    energyCalculationCount++;
                    if (energyCalculationCount >= energyCalculationInterval) {
                        readData.energy += readData.power * (energyCalculationInterval / 3600.0);
                        readData.mAh += readData.current * 1000 * (energyCalculationInterval / 3600.0);
                        // Serial.println(energyCalculationCount);
                        energyCalculationCount = 0;
                    }

                    // u8g2.setCursor(120,0);
                    // u8g2.print("x");
                }
            }
        }
    }
}
#define TIMER0_INTERVAL_MS 1000

// Init ESP32 timer 0 and 1
// ESP32Timer ITimer0(1);

// Create an instance of the U8g2 library.
// Use constructor that matches your setup (see https://github.com/olikraus/u8g2/wiki/u8g2setupcpp for details).
// This instance is used to call all the subsequent U8g2 functions (internally from GEM library,
// or manually in your sketch if it is required).
// Please update the pin numbers according to your setup. Use U8X8_PIN_NONE if the reset pin is not connected

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, /* reset=*/U8X8_PIN_NONE);
// Create menu object of class GEM_u8g2. Supply its constructor with reference to u8g2 object we created earlier

// Make protected fields accessible through custom getter methods

class GEMProxy : public GEM_u8g2 {
   public:
    using GEM_u8g2::GEM_u8g2;

    // Make _editValueMode accessible
    bool getEditValueMode() {
        return _editValueMode;
    }

    // Make _editValueType accessible
    int getEditValueType() {
        return _editValueType;
    }

    bool getEditValueTypeBool() {
        return _editValueType;
    }

    // Detect if cursor is at the last digit of editable variable
    bool reachedLastDigit() {
        return (_editValueCursorPosition == 0);
    }

    bool setEditValueCursorPosition(int position) {
        _editValueCursorPosition = position;
        _editValueVirtualCursorPosition = position;
        drawMenu();
        return true;
    }

    // void initEditValueCursor() override{
    //     _editValueCursorPosition = valueEditPrecision;
    //     _editValueVirtualCursorPosition = valueEditPrecision;
    //     drawMenu();
    // }

    byte getEditValueCursorPosition() {
        return _editValueCursorPosition;
    }

    void initEditValueCursor() override {
        _editValueCursorPosition = valueEditPrecision;  // assuming valueEditPrecision is defined
        _editValueVirtualCursorPosition = valueEditPrecision;
        drawMenu();
    }
};

GEMProxy menu(u8g2, /* menuPointerType= */ GEM_POINTER_ROW, /* menuItemsPerScreen= */ GEM_ITEMS_COUNT_AUTO, /* menuItemHeight= */ 8, /* menuPageScreenTopOffset= */ 10, /* menuValuesLeftOffset= */ 92);
// GEM_u8g2 menu(u8g2, GEM_POINTER_DASH, GEM_ITEMS_COUNT_AUTO, 10, 0, 86);

// Create variables that will be editable through the menu and assign them initial values
int interval = 500;
char label[GEM_STR_LEN] = "Blink!";  // Maximum length of the string should not exceed 16 characters
                                     // (plus special terminating character)!

// Supplementary variable used in millis based version of Blink routine
unsigned long previousMillis = 0;

// Variable to hold current label state (visible or hidden)
// bool holdOn = false;
bool inLoggingScreen = false;
bool inMainScreen = false;
bool exitNow = false;
unsigned long mainScreenStartMillis = 0;

// Create two menu item objects of class GEMItem, linked to interval and label variables
// with validateInterval() callback function attached to interval menu item,
// that will make sure that interval variable is within allowable range (i.e. >= 50)
// void validateInterval();  // Forward declaration
// GEMItem menuItemInterval("Interval:", interval, validateInterval);
// GEMItem menuItemLabel("Label:", label);

// Create menu button that will trigger blinkDelay() function. It will blink the label on the screen with delay()
// set to the value of interval variable. We will write (define) this function later. However, we should
// forward-declare it in order to pass to GEMItem constructor
// Likewise, create menu button that will trigger blinkMillis() function. It will blink the label on the screen with millis based
// delay set to the value of interval variable. We will write (define) this function later. However, we should
// forward-declare it in order to pass to GEMItem constructor

// Create menu page object of class GEMPage. Menu page holds menu items (GEMItem) and represents menu level.
// Menu can have multiple menu pages (linked to each other) with multiple menu items each
GEMPage menuPageMain("Main Menu");  // Main page

void resetEnergy();
void resetPeakVoltage();
void resetPeakCurrent();
void resetPeakPower();
void resetAll();

void mainScreenLoop() {
    if (exitNow == true) {
        menu.context.exit();
        exitNow = false;
    } else {
        // u8g2.firstPage();
        // do {
        unsigned long startMillis = micros();
        // readADC();
        displayMainScreen(0, hold == false ? readData : holdData);
        // Serial.println("Time taken: " + String((micros() - startMillis) / 1000));
        // } while (u8g2.nextPage());
    }
}

void mainScreenEnter() {
    u8g2.setFont(u8g2_font_helvB10_tf);
    inMainScreen = true;
    mainScreenStartMillis = millis();
    ADCPaused = false;
    // inLoggingScreen = true;
    u8g2.clear();
}
void mainScreenExit() {
    inMainScreen = false;
    menu.reInit();
    menu.drawMenu();
    menu.clearContext();
    menu.setMenuPageCurrent(menuPageMain);
}
void mainScreen() {
    menu.context.loop = mainScreenLoop;
    menu.context.enter = mainScreenEnter;
    menu.context.exit = mainScreenExit;
    menu.context.allowExit = false;  // Setting to false will require manual exit from the loop
    menu.context.enter();
}
GEMItem menuItemGoBack("Go back", mainScreen);

void loggingStatusScreen() {
    if (exitNow == true) {
        menu.context.exit();
        exitNow = false;
    } else {
        // u8g2.firstPage();
        // do {
        // unsigned long startMillis = micros();
        // readADC();
        displayLoggingStatus(logData, dataLoggingLastModifiedMillis);
        // Serial.println("Time taken: " + String((micros() - startMillis) / 1000));
        // } while (u8g2.nextPage());
    }
}
void loggingStatusEnter() {
    inMainScreen = true;
    u8g2.clear();
}
void loggingStatusExitt() {
    inMainScreen = false;
    menu.reInit();
    menu.drawMenu();
    menu.clearContext();
    menu.setMenuPageCurrent(menuPageMain);
}
void loggingStatus() {
    menu.context.loop = loggingStatusScreen;
    menu.context.enter = loggingStatusEnter;
    menu.context.exit = loggingStatusExitt;
    menu.context.allowExit = false;  // Setting to false will require manual exit from the loop
    menu.context.enter();
}

void exportLog() {
    delay(1000);
    menu.registerKeyPress(GEM_ITEM_BACK);
    menu.registerKeyPress(GEM_ITEM_BACK);
}

void selfCalibrationScreen() {
    ADCPaused = true;
    Serial.println("entered selfCalibrationScreen");
    u8g2.clearBuffer();  // clear the internal memory
    u8g2.setFont(u8g2_font_helvB10_tf);
    // u8g2.setFont(u8g2_font_ncenB10_tr);	// choose a suitable font
    u8g2.setCursor(0, 0);
    u8g2.print("Self Calibrating...");
    u8g2.sendBuffer();
    selfCalibrate();
    u8g2.setCursor(0, 30);
    u8g2.print("Done!");
    u8g2.sendBuffer();
    delay(1000);
    ADCPaused = false;
    menu.context.exit();
}
void selfCalibrateEnter() {
    u8g2.clear();
    // u8g2.clearBuffer();  // clear the internal memory
    u8g2.setFont(u8g2_font_helvB10_tf);
    // u8g2.setFont(u8g2_font_ncenB10_tr);	// choose a suitable font
    u8g2.setCursor(0, 0);
}
void selfCalibrateExit() {
    menu.reInit();
    menu.drawMenu();
    menu.clearContext();
    menu.setMenuPageCurrent(menuPageMain);
}
void selfCalibrationMenu() {
    menu.context.loop = selfCalibrationScreen;
    menu.context.enter = selfCalibrateEnter;
    menu.context.exit = selfCalibrateExit;
    menu.context.allowExit = false;  // Setting to false will require manual exit from the loop
    menu.context.enter();
}

void validateDisplayContrast() {
    if (displayContrast > 9) {
        displayContrast = 9;
    } else if (displayContrast < 0) {
        displayContrast = 0;
    }
    u8g2.setContrast(21 + (displayContrast * 26));
}

void loggingStatusChanged() {
    loggingStartedMessage();
    dataLoggingLastModifiedMillis = millis();
}

void applyHold() {
    // Serial.println("Hold: " + String(hold));
    if (hold == true) {
        pauseStopwatch();
        holdData.voltage = readData.voltage;
        holdData.current = readData.current;
        holdData.power = readData.power;
        holdData.energy = readData.energy;
        holdData.mAh = readData.mAh;
        holdData.peakCurrent = readData.peakCurrent;
        holdData.peakVoltage = readData.peakVoltage;
        holdData.peakPower = readData.peakPower;
        // holdData.elapsedMillis = 0;
    } else {
        resumeStopwatch();
    }
}

void applyLoggingInterval() {
    dataLogInterval = constrain(dataLogInterval, 2, 3600);
}
GEMPage menuPageDataLogging("Data Logging");  // Settings submenu
GEMItem menuItemLogData("Log Data", logData, loggingStatusChanged);
GEMItem menuItemLoggingInterval("Interval", dataLogInterval, applyLoggingInterval);
GEMItem menuItemLoggingStatus("Logging Status", loggingStatus);
GEMItem menuItemExportLog("Export Log", exportLog);

GEMPage menuPageAdvancedSettings("Advanced Settings");  // Settings submenu
GEMItem menuItemForceUse5mShunt("Use 5mR shunt", forceUse5mShunt);
GEMItem menuItemServerUpdateInterval("Server Update Interval", serverUpdateInterval);
GEMItem menuItemSelfCalibrate("Self Calibrate", selfCalibrationMenu);
GEMItem menuItemDisplayContrast("Disp. Contrast", displayContrast, validateDisplayContrast);
GEMItem menuItemMeasureOutputVoltage("Measure Output V:", readData.measureOutputVoltage);

GEMPage menuPageManualCalibration("Manual Calibration");    // Manual calibration submenu
GEMPage menuPageVoltageCalibration("Voltage Calibration");  // Voltage calibration submenu
GEMItem menuItemVoltage("Voltage Calibration", menuPageVoltageCalibration);
GEMItem menuItemVoltageOffset("Offset", calibration.voltageOffset);
GEMItem menuItemVoltageGain("Gain", calibration.voltageGainCurrent);

GEMPage menuPageLowVoltageAlarm("Low Voltage Alarm");
GEMItem menuItemAlarmOn("Alarm on", lowVoltageAlarm);
GEMItem menuItemAlarmVoltage("Alarm Voltage", alarmVoltage);

GEMPage menuPageResetValues("Reset Values");  // Settings submenu
GEMItem menuItemResetEnergy("Energy", resetEnergy);
GEMItem menuItemResetPeakVoltage("Peak Voltage", resetPeakVoltage);
GEMItem menuItemResetPeakCurrent("Peak Current", resetPeakCurrent);
GEMItem menuItemResetPeakPower("Peak Power", resetPeakPower);
GEMItem menuItemResetAll("Reset All", resetAll);

GEMItem menuItemHold("Hold:", hold, applyHold);
GEMItem menuItemResetValues("Reset Values", menuPageResetValues);
GEMItem menuItemDataLogging("Data Logging", menuPageDataLogging);
GEMItem menuItemAdvancedSettings("Advanced Settings", menuPageAdvancedSettings);
GEMItem menuItemLowVoltageAlarm("Low Voltage Alarm", menuPageLowVoltageAlarm);
GEMItem menuItemManualCalibration("Manual Calibration", menuPageManualCalibration);

// Which is equivalent to the following call (you can adjust parameters to better fit your screen if necessary):
// GEM_u8g2 menu(u8g2, /* menuPointerType= */ GEM_POINTER_ROW, /* menuItemsPerScreen= */ GEM_ITEMS_COUNT_AUTO, /* menuItemHeight= */ 10, /* menuPageScreenTopOffset= */ 10, /* menuValuesLeftOffset= */ 86);

void resetEnergy() {
    readData.energy = 0.0;
    menu.registerKeyPress(GEM_KEY_CANCEL);
}
void resetPeakVoltage() {
    readData.peakVoltage = 0.0;
    menu.registerKeyPress(GEM_KEY_CANCEL);
}
void resetPeakCurrent() {
    readData.peakCurrent = 0.0;
    menu.registerKeyPress(GEM_KEY_CANCEL);
}
void resetPeakPower() {
    readData.peakPower = 0.0;
    menu.registerKeyPress(GEM_KEY_CANCEL);
}
void resetAll() {
    readData.energy = 0.0;
    readData.peakVoltage = 0.0;
    readData.peakCurrent = 0.0;
    readData.peakPower = 0.0;
    resetStopwatch();
    menu.registerKeyPress(GEM_KEY_CANCEL);
}

void setupMenu() {
    // Add menu items to Settings menu page
    // menuPageResetValues.addMenuItem(menuItemInterval);
    menuPageResetValues.addMenuItem(menuItemResetEnergy);
    menuPageResetValues.addMenuItem(menuItemResetPeakVoltage);
    menuPageResetValues.addMenuItem(menuItemResetPeakCurrent);
    menuPageResetValues.addMenuItem(menuItemResetPeakPower);
    menuPageResetValues.addMenuItem(menuItemResetAll);

    menuPageDataLogging.addMenuItem(menuItemLogData);
    menuPageDataLogging.addMenuItem(menuItemLoggingInterval);
    menuPageDataLogging.addMenuItem(menuItemLoggingStatus);
    // menuPageDataLogging.addMenuItem(menuItemStartLogging);
    // menuPageDataLogging.addMenuItem(menuItemStopLogging);
    menuPageDataLogging.addMenuItem(menuItemExportLog);

    // Add menu items to Main Menu page
    // menuPageMain.addMenuItem(menuItemVoltageOffset);
    // menuPageMain.addMenuItem(menuItemVoltageGain);
    menuPageMain.addMenuItem(menuItemGoBack);
    menuPageMain.addMenuItem(menuItemHold);
    menuPageMain.addMenuItem(menuItemDataLogging);
    menuPageMain.addMenuItem(menuItemLowVoltageAlarm);
    menuPageMain.addMenuItem(menuItemResetValues);
    menuPageMain.addMenuItem(menuItemAdvancedSettings);

    menuPageAdvancedSettings.addMenuItem(menuItemMeasureOutputVoltage);
    menuPageAdvancedSettings.addMenuItem(menuItemForceUse5mShunt);
    menuPageAdvancedSettings.addMenuItem(menuItemServerUpdateInterval);
    menuPageAdvancedSettings.addMenuItem(menuItemSelfCalibrate);
    menuPageAdvancedSettings.addMenuItem(menuItemDisplayContrast);
    menuPageAdvancedSettings.addMenuItem(menuItemManualCalibration);

    menuPageManualCalibration.addMenuItem(menuItemVoltage);

    menuPageVoltageCalibration.addMenuItem(menuItemVoltageOffset);
    menuPageVoltageCalibration.addMenuItem(menuItemVoltageGain);

    // Specify parent menu page for the Settings menu page
    menuPageResetValues.setParentMenuPage(menuPageMain);
    menuPageDataLogging.setParentMenuPage(menuPageMain);
    menuPageAdvancedSettings.setParentMenuPage(menuPageMain);
    menuPageManualCalibration.setParentMenuPage(menuPageAdvancedSettings);
    menuPageVoltageCalibration.setParentMenuPage(menuPageManualCalibration);

    menuPageLowVoltageAlarm.setParentMenuPage(menuPageMain);
    menuPageLowVoltageAlarm.addMenuItem(menuItemAlarmOn);
    menuPageLowVoltageAlarm.addMenuItem(menuItemAlarmVoltage);

    menuItemVoltageOffset.setPrecision(3);
    // menuItemVoltageGain.setPrecision(6);

    // Add Main Menu page to menu and set it as current
    menu.setMenuPageCurrent(menuPageMain);
    menu.setDrawMenuCallback(drawBatteryStatus);
}

void setPressed() {
    if (millis() - mainScreenStartMillis > 1000 && inMainScreen == true) {
        // delay(100);
        if (inMainScreen == true) {
            exitNow = true;
            // menu.registerKeyPress(GEM_KEY_CANCEL);
            // inMainScreen = false;
        }
    }
    // delay(200);
}

void setup() {
    Wire.begin(1, 3);
    Wire.setClock(1000000L);

    // U8g2 library init. Pass pin numbers the buttons are connected to.
    // The push-buttons should be wired with pullup resistors (so the LOW means that the button is pressed)
    // u8g2.begin(U8X8_PIN_NONE, U8X8_PIN_NONE, U8X8_PIN_NONE, U8X8_PIN_NONE, U8X8_PIN_NONE, U8X8_PIN_NONE);
    u8g2.begin();

    u8g2.clearBuffer();
    u8g2.drawXBMP((128 - splash_width) / 2, 0, splash_height, splash_width, splash_bits);
    u8g2.sendBuffer();

    xTaskCreatePinnedToCore(
        backgroundTask,   /* Function to implement the task */
        "backgroundTask", /* Name of the task */
        10000,            /* Stack size in words */
        NULL,             /* Task input parameter */
        0,                /* Priority of the task */
        &task1,           /* Task handle. */
        0);               /* Core where the task should run */
    initButtons();
    setButtonCallback(setPressed);

    // Serial communication setup
    Serial.begin(256000);
    Serial.println("Hello World!");
    serverSetup();
    googleSheetSetup();

    initADC();
    ADCInitialized = true;
    previousADCReadMicros = micros();

    menu.setSplashDelay(0);  // Set splash screen delay to 0 to disable it
    menu.init();
    setupMenu();
    mainScreen();
    startStopwatch();

    tone(BUZZER_PIN, TONE_FREQUENCY, TONE_DURATION);
    delay(TONE_DURATION + 100);
    tone(BUZZER_PIN, TONE_FREQUENCY, TONE_DURATION);
    delay(TONE_DURATION + 100);
    tone(BUZZER_PIN, TONE_FREQUENCY, TONE_DURATION);

    // attachInterrupt(digitalPinToInterrupt(SET_BUTTON_PIN), handleSetButtonPressed, RISING);
}

unsigned long lastSerPrintMillis = 0;
void loop() {
    if (millis() - lastSerPrintMillis > 2000) {
        lastSerPrintMillis = millis();
        Serial.println("Flash size: " + String(ESP.getFlashChipSize()/1000));
        Serial.println("Total heap: " + String(ESP.getHeapSize()/1000));
        Serial.println("Free heap: " + String(ESP.getFreeHeap()/1000));
        Serial.println("Used heap: " + String((ESP.getHeapSize() - ESP.getFreeHeap())/1000));
        Serial.println("Total psram: " + String(ESP.getPsramSize()/1000));
        Serial.println("Free psram: " + String(ESP.getFreePsram()/1000));
    }
    handleButtons();
    serverLoop(readData);
    googleSheetLoop(readData);
    // If menu is ready to accept button press...
    if (menu.readyForKey()) {
        // ...detect key press using KeyDetector library (native U8g2 method of key detection can be adapted to this as well)
        myKeyDetector.detect();
        // Pass pressed button to menu based on current state of the menu
        // (pressed button ID is stored in trigger property of KeyDetector object)

        if (myKeyDetector.trigger == GEM_KEY_OK) {
            // Serial.print("Edit mode: ");
            // Serial.print(menu.getEditValueMode());
            // Serial.print(", Edit type: ");
            // Serial.println(menu.getEditValueType());
            // If Enter button is pressed while in edit mode...
            if (menu.getEditValueMode() == true && (menu.getEditValueType() != GEM_VAL_SELECT)) {
                Serial.println("reached last digit: " + (String)menu.reachedLastDigit() + ", cursor position: " + (String)menu.getEditValueCursorPosition());
                menu.registerKeyPress(menu.reachedLastDigit() ? GEM_KEY_OK : GEM_KEY_LEFT);
            } else {
                // ...else as GEM_KEY_OK is pressed, pass it to menu
                menu.registerKeyPress(GEM_KEY_OK);
            }
        } else {
            // ...else detect other buttons as usual
            switch (myKeyDetector.trigger) {
                case GEM_KEY_UP:
                    menu.registerKeyPress(GEM_KEY_UP);
                    Serial.println("Up");
                    break;
                case GEM_KEY_DOWN:
                    menu.registerKeyPress(GEM_KEY_DOWN);
                    Serial.println("Down");
                    break;
                case GEM_KEY_OK:
                    menu.registerKeyPress(GEM_KEY_OK);
                    Serial.println("OK");
                    break;
                case GEM_KEY_CANCEL:
                    menu.registerKeyPress(GEM_KEY_CANCEL);
                    Serial.println("CANCEL");
                    break;
                case GEM_KEY_LEFT:
                    menu.registerKeyPress(GEM_KEY_LEFT);
                    Serial.println("LEFT");
                    break;
                case GEM_KEY_RIGHT:
                    menu.registerKeyPress(GEM_KEY_RIGHT);
                    Serial.println("RIGHT");
                    break;

                default:
                    // Serial.println("something else");
                    break;
            }
        }
    }
}

void loggingStartedMessage() {
    // Set new title and redraw menu
    menuItemLogData.setTitle("Started!");
    menu.drawMenu();

    // Set delay for new title to be visible
    delay(1000);

    // Set original title back and redraw menu
    menuItemLogData.setTitle("Log Data");
    menu.drawMenu();
}