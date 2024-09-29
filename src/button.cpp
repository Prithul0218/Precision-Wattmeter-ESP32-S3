#include <Arduino.h>
#include <button.h>
// #define UP_BUTTON_PIN 39
// #define SET_BUTTON_PIN 35
// #define DOWN_BUTTON_PIN 33
// #define BUZZER_PIN 14

// // Define the frequency and duration of the tone
// #define TONE_FREQUENCY 4000  // Hz
// #define TONE_DURATION 100    // milliseconds

// Global variable to hold the function pointer
ButtonCallback buttonCallback = NULL;

// Array of Key objects that will link GEM key identifiers with dedicated pins
Key keys[] = {{GEM_KEY_UP, UP_BUTTON_PIN}, {GEM_KEY_DOWN, DOWN_BUTTON_PIN}, {GEM_KEY_OK, SET_BUTTON_PIN}};

// Create KeyDetector object
KeyDetector myKeyDetector(keys, sizeof(keys)/sizeof(Key), /* debounceDelay= */ 10, /*analogThreshold*/ 0, /*pullup*/ true);

// Function to set the callback function
void setButtonCallback(ButtonCallback callback) {
    buttonCallback = callback;
}

volatile bool upButtonPressed = false;
volatile bool setButtonPressed = false;
volatile bool downButtonPressed = false;

unsigned long lastUpButtonTime = 0;
unsigned long lastSetButtonTime = 0;
unsigned long lastDownButtonTime = 0;
unsigned long debounceDelay = 200;  // milliseconds

void IRAM_ATTR handleUpButtonInterrupt() {
    if (!upButtonPressed && (millis() - lastUpButtonTime) > debounceDelay) {
        upButtonPressed = true;
        lastUpButtonTime = millis();
        tone(BUZZER_PIN, TONE_FREQUENCY, TONE_DURATION);
    }
}

void IRAM_ATTR handleSetButtonInterrupt() {
    if (!setButtonPressed && (millis() - lastSetButtonTime) > debounceDelay) {
        setButtonPressed = true;
        lastSetButtonTime = millis();
        tone(BUZZER_PIN, TONE_FREQUENCY, TONE_DURATION);
        if (buttonCallback != NULL) {
            // Call the callback function
            buttonCallback();
        }
    }
}

void IRAM_ATTR handleDownButtonInterrupt() {
    if (!downButtonPressed && (millis() - lastDownButtonTime) > debounceDelay) {
        downButtonPressed = true;
        lastDownButtonTime = millis();
        tone(BUZZER_PIN, TONE_FREQUENCY, TONE_DURATION);
    }
}

void initButtons() {
    // Set button pins as input with pullup resistors enabled
    pinMode(UP_BUTTON_PIN, INPUT_PULLUP);
    pinMode(SET_BUTTON_PIN, INPUT_PULLUP);
    pinMode(DOWN_BUTTON_PIN, INPUT_PULLUP);

    // Set buzzer pin as output
    pinMode(BUZZER_PIN, OUTPUT);

    // Attach interrupts to the pins with RISING edge trigger
    attachInterrupt(digitalPinToInterrupt(UP_BUTTON_PIN), handleUpButtonInterrupt, RISING);
    attachInterrupt(digitalPinToInterrupt(SET_BUTTON_PIN), handleSetButtonInterrupt, RISING);
    attachInterrupt(digitalPinToInterrupt(DOWN_BUTTON_PIN), handleDownButtonInterrupt, RISING);
}

void handleButtons() {
    if (upButtonPressed || setButtonPressed || downButtonPressed) {
        // Generate the tone
        // tone(BUZZER_PIN, TONE_FREQUENCY, TONE_DURATION);

        // Serial print which button was pressed
        // if (upButtonPressed) {
        //     Serial.println("Up button pressed");
        // }
        // if (setButtonPressed) {
        //     Serial.println("Set button pressed");
        // }
        // if (downButtonPressed) {
        //     Serial.println("Down button pressed");
        // }

        // Reset the flags after debounce delay
        // delay(debounceDelay);
        upButtonPressed = false;
        setButtonPressed = false;
        downButtonPressed = false;
    }
}