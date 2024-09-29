#pragma once
#include <Arduino.h>
#include <KeyDetector.h>
#include <GEM_u8g2.h>

#define UP_BUTTON_PIN 43
#define SET_BUTTON_PIN 36
#define DOWN_BUTTON_PIN 35
#define BUZZER_PIN 14

// Define the frequency and duration of the tone
#define TONE_FREQUENCY 4000  // Hz
#define TONE_DURATION 100    // milliseconds

typedef void (*ButtonCallback)();

extern Key keys[];
extern KeyDetector myKeyDetector;

// Function to set the callback function
void setButtonCallback(ButtonCallback callback);
extern volatile bool upButtonPressed;
extern volatile bool setButtonPressed;
extern volatile bool downButtonPressed;

extern unsigned long lastUpButtonTime;
extern unsigned long lastSetButtonTime;
extern unsigned long lastDownButtonTime;
extern unsigned long debounceDelay;  // milliseconds

void IRAM_ATTR handleUpButtonInterrupt();
void IRAM_ATTR handleSetButtonInterrupt();
void IRAM_ATTR handleDownButtonInterrupt();

void initButtons();
void handleButtons();